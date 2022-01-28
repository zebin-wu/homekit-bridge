// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <esp_system.h>
#include <esp_console.h>
#include <argtable3/argtable3.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_timer.h>

#define APP_WIFI_RETRY_TIME  5
#define APP_WIFI_RECONN_INTEVAL_MS (30 * 1000)

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
    esp_timer_handle_t reconn_timer;
    bool got_ipv4;
    bool got_ipv6;
    void (*connected_cb)(void);
} app_wifi_desc;

static const char *TAG = "app_wifi";

static app_wifi_desc gv_wifi_desc;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    static int retry = 0;
    char ssid[32 + 1];

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t *evt = event_data;
            ESP_LOGW(TAG, "Disconnected, reason: %d", evt->reason);
            if (retry < APP_WIFI_RETRY_TIME) {
                retry++;
                ESP_LOGI(TAG, "Trying to reconnect ...");
                esp_wifi_connect();
            } else {
                ESP_LOGI(TAG, "Reconnect after %dms ...", APP_WIFI_RECONN_INTEVAL_MS);
                esp_timer_start_once(gv_wifi_desc.reconn_timer,
                    APP_WIFI_RECONN_INTEVAL_MS * 1000);
            }
            gv_wifi_desc.got_ipv4 = false;
            gv_wifi_desc.got_ipv6 = false;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            wifi_event_sta_connected_t *evt = event_data;
            memcpy(ssid, evt->ssid, evt->ssid_len);
            ssid[evt->ssid_len] = '\0';
            ESP_LOGI(TAG, "Connected to AP \"%s\", channel: %u", ssid, evt->channel);
            ESP_ERROR_CHECK(esp_netif_create_ip6_linklocal((esp_netif_t *)arg));
            break;
        }
        default:
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *evt = event_data;
            ESP_LOGI(TAG, "Got IPv4 address: " IPSTR, IP2STR(&evt->ip_info.ip));
            retry = 0;
            gv_wifi_desc.got_ipv4 = true;
            if (gv_wifi_desc.got_ipv6 && gv_wifi_desc.connected_cb) {
                gv_wifi_desc.connected_cb();
            }
            break;
        }
        case IP_EVENT_GOT_IP6:
        {
            ip_event_got_ip6_t *evt = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "Got IPv6 address: " IPV6STR, IPV62STR(evt->ip6_info.ip));
            gv_wifi_desc.got_ipv6 = true;
            if (gv_wifi_desc.got_ipv4 && gv_wifi_desc.connected_cb) {
                gv_wifi_desc.connected_cb();
            }
            break;
        }
        default:
            break;
        }
    }
}

static void app_wifi_reconn_timer_cb(void* arg) {
    ESP_LOGI(TAG, "Trying to reconnect ...");
    esp_wifi_connect();
}

void app_wifi_init(void)
{
    assert(gv_wifi_desc.state == APP_WIFI_STATE_NOINIT);

    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("wifi_init", ESP_LOG_WARN);
    esp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, event_handler, netif));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

    esp_timer_create_args_t timer_conf = {
        .callback = app_wifi_reconn_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_conf, &gv_wifi_desc.reconn_timer));

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
    ESP_ERROR_CHECK(esp_wifi_connect());
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
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = app_wifi_join_cmd,
        .argtable = &join_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
}

void app_wifi_set_connected_cb(void (*cb)(void)) {
    gv_wifi_desc.connected_cb = cb;
}
