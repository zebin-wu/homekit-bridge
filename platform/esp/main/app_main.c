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
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_console.h>

#include <app.h>
#include <pal/cli.h>
#include <pal/cli_int.h>
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
#define APP_NVS_NAMESPACE_NAME "bridge"
#define APP_NVS_LOG_ENABLED_TYPE "log"

/** Arguments used by 'log' function */
static struct {
    struct arg_str *enabled_type;
    struct arg_end *end;
} log_args;

static int app_log_cmd(int argc, char **argv, void *ctx) {
    int nerrors = arg_parse(argc, argv, (void **) &log_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, log_args.end, argv[0]);
        return 1;
    }

    const char *enabled_type_strs[] = {
        [kHAPPlatformLogEnabledTypes_None] = "none",
        [kHAPPlatformLogEnabledTypes_Default] = "default",
        [kHAPPlatformLogEnabledTypes_Info] = "info",
        [kHAPPlatformLogEnabledTypes_Debug] = "debug"
    };

    if (log_args.enabled_type->count == 0) {
        printf("Current enabled log type: %s\n",
            enabled_type_strs[HAPPlatformLogGetEnabledTypes(NULL)]);
        return 0;
    }

    for (int i = 0; i < HAPArrayCount(enabled_type_strs); i++) {
        if (HAPStringAreEqual(argv[1], enabled_type_strs[i])) {
            HAPPlatformLogSetEnabledTypes(NULL, i);
            nvs_handle_t nvs_handle;
            ESP_ERROR_CHECK(nvs_open(APP_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle));
            esp_err_t err = nvs_set_u8(nvs_handle, APP_NVS_LOG_ENABLED_TYPE, i);
            nvs_close(nvs_handle);
            return err;
        }
    }

    printf("log: invalid command\n");
    return -1;
}

void app_register_log_cmd() {
    log_args.enabled_type = arg_str0(NULL, NULL, "<none|default|info|debug>", "Log enabled type");
    log_args.end = arg_end(1);

    pal_err err = pal_cli_register(&(pal_cli_info) {
        .cmd = "log",
        .help = "Show or set enabled log type",
        .argtable = &log_args,
        .func = app_log_cmd,
    }, NULL);
    if (err != PAL_ERR_OK) {
        HAPFatalError();
    }
}

void app_main_task(void *arg) {
    // Initialize pal modules.
    HAPPlatformRunLoopCreate();
    pal_cli_init();
    pal_ssl_init();
    pal_dns_init();
    pal_net_if_init();

    app_register_log_cmd();

    // Initialize application.
    app_init(APP_SPIFFS_DIR_PATH, CONFIG_LUA_APP_ENTRY);

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // De-initialize application.
    app_deinit();

    // De-initialize pal modules.
    pal_net_if_init();
    pal_dns_deinit();
    pal_ssl_deinit();
    pal_cli_deinit();
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

    // Get log enabled type from NVS
    nvs_handle_t nvs_handle;
    uint8_t log_enabled_type;
    ESP_ERROR_CHECK(nvs_open(APP_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle));
    ret = nvs_get_u8(nvs_handle, APP_NVS_LOG_ENABLED_TYPE, &log_enabled_type);
    nvs_close(nvs_handle);
    if (ret == ESP_OK) {
        HAPPlatformLogSetEnabledTypes(NULL, log_enabled_type);
    }

    app_console_init();
    app_wifi_init();
    app_wifi_on();
    app_wifi_register_cmd();
    app_spiffs_init(APP_SPIFFS_DIR_PATH);
    app_console_start();

    xTaskCreate(app_main_task, "app", APP_MAIN_TASK_STACKSIZE,
        NULL, APP_MAIN_TASK_PRIORITY, NULL);
}
