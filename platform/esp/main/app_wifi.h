// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <esp_err.h>

/**
 * Initialize Wi-Fi.
*/
void app_wifi_init(void);

/**
 * Connect to network.
*/
esp_err_t app_wifi_connect(void);

#endif /* APP_WIFI_H */
