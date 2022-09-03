// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <sys/queue.h>
#include <esp_console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <pal/cli.h>
#include <pal/memory.h>
#include <HAPBase.h>
#include <HAPPlatformRunLoop.h>

struct pal_cli_cmd {
    const char *cmd;
    int (*func)(int argc, char *argv[], void *ctx);
    void *ctx;
    SLIST_ENTRY(pal_cli_cmd) list_entry;
};

struct pal_cli_run_ctx {
    struct pal_cli_cmd *cmd;
    int argc;
    char **argv;
};

static esp_console_repl_t *grepl;
static bool ginited;
static QueueHandle_t gretq;
static SLIST_HEAD(, pal_cli_cmd) glist_head;

void pal_cli_init() {
    HAPPrecondition(ginited == false);
    HAPPrecondition(grepl == NULL);

    gretq = xQueueCreate(1, sizeof(int));
    HAPPrecondition(gretq);
    SLIST_INIT(&glist_head);
    ginited = true;
}

void pal_cli_deinit() {
    HAPPrecondition(ginited = true);

    for (struct pal_cli_cmd *t = SLIST_FIRST(&glist_head); t;) {
        struct pal_cli_cmd *cur = t;
        t = SLIST_NEXT(t, list_entry);
        pal_mem_free(cur);
    }
    SLIST_INIT(&glist_head);
    vQueueDelete(gretq);
    ginited = false;
}

static void pal_cli_run_cb(void *context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(struct pal_cli_run_ctx));

    struct pal_cli_run_ctx *ctx = context;
    int ret = ctx->cmd->func(ctx->argc, ctx->argv, ctx->cmd->ctx);
    xQueueSend(gretq, &ret, portMAX_DELAY);
}

static int pal_cli_run(struct pal_cli_cmd *cmd, int argc, char **argv) {
    struct pal_cli_run_ctx ctx = {
        .cmd = cmd,
        .argc = argc,
        .argv = argv,
    };
    if (HAPPlatformRunLoopScheduleCallback(pal_cli_run_cb, &ctx, sizeof(ctx)) != kHAPError_None) {
        return -1;
    }
    int ret = -1;
    xQueueReceive(gretq, &ret, portMAX_DELAY);
    return ret;
}

static int pal_cli_cmd_func(int argc, char *argv[]) {
    if (ginited == false) {
        return -1;
    }

    struct pal_cli_cmd *t;
    SLIST_FOREACH(t, &glist_head, list_entry) {
        if (!strcmp(argv[0], t->cmd)) {
            return pal_cli_run(t, argc, argv);
        }
    }

    HAPFatalError();
}

pal_err pal_cli_register(const pal_cli_info *info, void *ctx) {
    HAPPrecondition(ginited);
    HAPPrecondition(info);
    HAPPrecondition(info->cmd);
    HAPPrecondition(info->func);

    esp_err_t err = esp_console_cmd_register(&(esp_console_cmd_t) {
        .command = info->cmd,
        .help = info->help,
        .hint = info->hint,
        .argtable = info->argtable,
        .func = pal_cli_cmd_func,
    });
    switch (err) {
    case ESP_OK: {
        struct pal_cli_cmd *cmd = pal_mem_alloc(sizeof(*cmd));
        if (!cmd) {
            return PAL_ERR_ALLOC;
        }
        cmd->cmd = info->cmd;
        cmd->func = info->func;
        cmd->ctx = ctx;
        SLIST_INSERT_HEAD(&glist_head, cmd, list_entry);
        return PAL_ERR_OK;
    }
    case ESP_ERR_NO_MEM:
        return PAL_ERR_ALLOC;
    case ESP_ERR_INVALID_ARG:
        return PAL_ERR_INVALID_ARG;
    default:
        return PAL_ERR_UNKNOWN;
    }
}
