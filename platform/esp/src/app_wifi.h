// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <esp_err.h>

/**
 * Initialize Wi-Fi.
 */
void app_wifi_init(void);

/**
 * Turn on the Wi-Fi.
 */
void app_wifi_on(void);

/**
 * Turn off the Wi-Fi.
 */
void app_wifi_off(void);

/**
 * Connect to network.
 */
void app_wifi_connect(const char *ssid, const char *password);

/**
 * Set connected callback.
 */
void app_wifi_set_connected_cb(void (*cb)(void));

/**
 * Regsiter Wi=Fi command.
 */
void app_wifi_register_cmd(void);

#endif /* APP_WIFI_H */
