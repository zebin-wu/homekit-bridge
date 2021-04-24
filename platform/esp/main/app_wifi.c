// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <esp_system.h>
#include <esp_console.h>
#include <argtable3/argtable3.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#define APP_ESP_MAXIMUM_RETRY  CONFIG_APP_MAXIMUM_RETRY

/*
 * Wifi state.
 *                 NOINIT
 *                   | call app_wifi_init
 *                   V
 *                 INITED
 *                  A | call app_wifi_on
 * call app_wifi_off| | V
 *                   ON
 */
typedef enum {
    APP_WIFI_STATE_NOINIT,  /* not inited state */
    APP_WIFI_STATE_INITED,  /* inited state */
    APP_WIFI_STATE_ON,      /* wifi on state */
} ap_wifi_state;

typedef struct {
    ap_wifi_state state;
    wifi_mode_t mode;
} app_wifi_desc;

static const char *TAG = "wifi";

static app_wifi_desc gv_wifi_desc;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static int retry = 0;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:;
            wifi_event_sta_disconnected_t *event = event_data;
            ESP_LOGI(TAG, "disconnected from AP, reason: %d", event->reason);
            if (retry < APP_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                retry++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:;
            ip_event_got_ip_t *event = event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            retry = 0;
            break;
        default:
            break;
        }
    }
}            

void app_wifi_init(void)
{
    assert(gv_wifi_desc.state == APP_WIFI_STATE_NOINIT);

    esp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_netif_init());

    assert(esp_netif_create_default_wifi_sta());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    gv_wifi_desc.state = APP_WIFI_STATE_INITED;
    ESP_LOGI(TAG, "noinit -> inited");
}

void app_wifi_on(void)
{
    assert(gv_wifi_desc.state == APP_WIFI_STATE_INITED);
    ESP_ERROR_CHECK(esp_wifi_start());
    gv_wifi_desc.state = APP_WIFI_STATE_ON;
    ESP_LOGI(TAG, "inited -> on");
}

void app_wifi_off(void)
{
    assert(gv_wifi_desc.state == APP_WIFI_STATE_ON);
    ESP_ERROR_CHECK(esp_wifi_stop());
    gv_wifi_desc.state = APP_WIFI_STATE_INITED;
    ESP_LOGI(TAG, "on -> inited");
}

void app_wifi_connect(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password) {
        strncpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password));
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}

/** Arguments used by 'join' function */
static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

static int app_wifi_join_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    app_wifi_connect(join_args.ssid->sval[0], 
        join_args.password->sval[0]);
    return 0;
}

void app_wifi_register_cmd(void)
{
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<password>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "wifi_join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = app_wifi_join_cmd,
        .argtable = &join_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
}
