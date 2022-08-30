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
    int (*func)(int argc, char *argv[]);
    SLIST_ENTRY(pal_cli_cmd) list_entry;
};

struct pal_cli_run_ctx {
    QueueHandle_t ret_que;
    struct pal_cli_cmd *cmd;
    int argc;
    char **argv;
};

static bool ginited;
static SLIST_HEAD(, pal_cli_cmd) glist_head;

void pal_cli_init() {
    HAPPrecondition(ginited == false);

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
    ginited = false;
}

static void pal_cli_run_cb(void *context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(struct pal_cli_run_ctx));

    struct pal_cli_run_ctx *ctx = context;
    int ret = ctx->cmd->func(ctx->argc, ctx->argv);
    xQueueSend(ctx->ret_que, &ret, portMAX_DELAY);
}

static int pal_cli_run(struct pal_cli_cmd *cmd, int argc, char **argv) {
    QueueHandle_t ret_que = xQueueCreate(1, sizeof(int));
    if (ret_que == NULL) {
        return -1;
    }

    struct pal_cli_run_ctx ctx = {
        .ret_que = ret_que,
        .cmd = cmd,
        .argc = argc,
        .argv = argv,
    };
    if (HAPPlatformRunLoopScheduleCallback(pal_cli_run_cb, &ctx, sizeof(ctx)) != kHAPError_None) {
        vQueueDelete(ret_que);
        return -1;
    }
    int ret = -1;
    xQueueReceive(ret_que, &ret, portMAX_DELAY);
    vQueueDelete(ret_que);
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

pal_err pal_cli_register(const pal_cli_info *info) {
    HAPPrecondition(ginited);
    HAPPrecondition(info);
    HAPPrecondition(info->cmd);
    HAPPrecondition(info->func);
    esp_err_t err = esp_console_cmd_register(&(esp_console_cmd_t) {
        .command = info->cmd,
        .help = info->help,
        .hint = info->hint,
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
