// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_console.h>
#include <esp_heap_caps.h>

#include "cmd_system.h"
#include "app_console.h"

#if __has_include("esp_psram.h")
#include "esp_psram.h"
#define HAS_ESP_PSRAM_H 1
#else
#define HAS_ESP_PSRAM_H 0
#endif

#define APP_CONSOLE_TASK_STACKSIZE  3 * 1024

#define PROMPT_STR CONFIG_IDF_TARGET

static esp_console_repl_t *grepl;

static void print_mem_line(const char *name, uint32_t caps)
{
    size_t total   = heap_caps_get_total_size(caps);
    size_t free    = heap_caps_get_free_size(caps);
    size_t minfree = heap_caps_get_minimum_free_size(caps);
    size_t largest = heap_caps_get_largest_free_block(caps);
    size_t used    = (total >= free) ? (total - free) : 0;

    printf("%-8s %12u B %12u B %12u B %12u B %12u B\r\n",
           name,
           (unsigned)total,
           (unsigned)used,
           (unsigned)free,
           (unsigned)minfree,
           (unsigned)largest);
}

static bool psram_available(void)
{
#if HAS_ESP_PSRAM_H
    return esp_psram_is_initialized();
#else
    return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0;
#endif
}

static int free_cmd_func(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("%-8s %14s %14s %14s %14s %14s\r\n",
           "", "total", "used", "free", "minfree", "largest");

    print_mem_line("SRAM", MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (psram_available()) {
        print_mem_line("PSRAM", MALLOC_CAP_SPIRAM);
    }

    return 0;
}

esp_err_t register_free_command(void)
{
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Show SRAM/PSRAM usage",
        .hint = NULL,
        .func = &free_cmd_func,
        .argtable = NULL,
    };

    return esp_console_cmd_register(&cmd);
}

void app_console_init(void)
{
    assert(grepl == NULL);
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    repl_config.prompt = PROMPT_STR ">";
    repl_config.task_stack_size = APP_CONSOLE_TASK_STACKSIZE;

    /* Register commands */
    esp_console_register_help_command();
    register_system();
    register_free_command();

    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &grepl));
}

void app_console_start(void)
{
    assert(grepl);
    ESP_ERROR_CHECK(esp_console_start_repl(grepl));
}
