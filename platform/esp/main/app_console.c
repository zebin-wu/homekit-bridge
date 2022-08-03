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
#include <cmd_system.h>

#include "app_console.h"

#define APP_CONSOLE_TASK_STACKSIZE  3 * 1024

#define PROMPT_STR CONFIG_IDF_TARGET

static esp_console_repl_t *grepl;

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

    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &grepl));
}

void app_console_start(void)
{
    assert(grepl);
    ESP_ERROR_CHECK(esp_console_start_repl(grepl));
}
