// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.
//
// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_console.h>

#include <app.h>
#include <argtable3.h>
#include <pal/ssl.h>
#include <pal/ssl_int.h>
#include <pal/dns.h>
#include <pal/net_if_int.h>

#include <HAPPlatformRunLoop+Init.h>
#include <HAPPlatformLog+Init.h>

#include "app_wifi.h"
#include "app_console.h"
#include "app_spiffs.h"

#define APP_MAIN_TASK_STACKSIZE 8 * 1024
#define APP_MAIN_TASK_PRIORITY 1
#define APP_SPIFFS_DIR_PATH "/spiffs"
#define APP_EXEC_NAME "homekit-bridge"

static QueueHandle_t gexec_retq;
static struct {
    struct arg_str *cmd;
    struct arg_str *args;
    struct arg_end *end;
} app_exec_args;

static void app_default_returned(pal_err err, void *arg) {
    if (err == PAL_ERR_UNKNOWN) {
        app_exit();
    }
}

static void app_returned(pal_err err, void *arg) {
    int ret = err == PAL_ERR_OK ? 0 : 1;
    xQueueSend(gexec_retq, &ret, portMAX_DELAY);
}

static int app_exec_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &app_exec_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, app_exec_args.end, argv[0]);
        return 1;
    }

    int ret;
    app_exec(app_exec_args.cmd->sval[0],
        app_exec_args.args->count,
        app_exec_args.args->sval, app_returned, NULL);
    xQueueReceive(gexec_retq, &ret, portMAX_DELAY);
    return ret;
}

static void app_register_exec_cmd() {
    gexec_retq = xQueueCreate(1, sizeof(int));

    app_exec_args.cmd = arg_str1(NULL, NULL, "<cmd>", "command");
    app_exec_args.args = arg_strn(NULL, NULL, "<arg>", 0, 16, "command argument");
    app_exec_args.end = arg_end(2);

    const esp_console_cmd_t exec_cmd = {
        .command = APP_EXEC_NAME,
        .help = "Execute a command",
        .hint = NULL,
        .func = app_exec_cmd,
        .argtable = &app_exec_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&exec_cmd));
}

void app_main_task(void *arg) {
    // Initialize pal modules.
    HAPPlatformRunLoopCreate();
    pal_ssl_init();
    pal_dns_init();
    pal_net_if_init();

    // Initialize application.
    app_init(APP_SPIFFS_DIR_PATH);
    app_register_exec_cmd();

    // Execute default command.
    app_exec(APP_EXEC_DEFAULT_CMD, APP_EXEC_DEFAULT_ARGC, APP_EXEC_DEFAULT_ARGV, app_default_returned, NULL);

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // De-initialize application.
    app_deinit();

    // De-initialize pal modules.
    pal_net_if_init();
    pal_dns_deinit();
    pal_ssl_deinit();
    HAPPlatformRunLoopRelease();
}

void app_main() {
    // Initialize default NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    app_console_init();
    app_wifi_init();
    app_wifi_on();
    app_wifi_register_cmd();
    app_spiffs_init(APP_SPIFFS_DIR_PATH);
    app_console_start();

    xTaskCreate(app_main_task, "app", APP_MAIN_TASK_STACKSIZE,
        NULL, APP_MAIN_TASK_PRIORITY, NULL);
}
