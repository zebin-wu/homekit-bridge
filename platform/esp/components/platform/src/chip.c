// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/chip.h>
#include <esp_system.h>

#define SERIAL_NUMBER_BUF_LEN (6 * 2 + 1)
#define HARDWARE_VERSION_BUF_LEN 16

static char serial_number[SERIAL_NUMBER_BUF_LEN];
static char hardware_version[HARDWARE_VERSION_BUF_LEN];

/*
 * Format 48-bit MAC address into the user-supplied buffer.
 * Buffer should be at least 13 bytes long.
 * String will be NUL-terminated and truncated if necessary.
 */
static char *format_mac(const uint8_t *mac, char *buf, size_t buf_len) {
    snprintf(buf, buf_len, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

const char *pal_chip_get_manufacturer(void) {
    return "Espressif";
}

const char *pal_chip_get_model(void) {
    const char *model;
    esp_chip_info_t info;

    esp_chip_info(&info);
    switch (info.model) {
    case CHIP_ESP32:
        model = "ESP32";
        break;
    case CHIP_ESP32S2:
        model = "ESP32-S2";
        break;
    case CHIP_ESP32S3:
        model = "ESP32-S3";
        break;
    case CHIP_ESP32C3:
        model = "ESP32-C3";
        break;
    default:
        model = "Unknown";
        break;
    }
    return model;
}

const char *pal_chip_get_serial_number(void) {
    if (serial_number[0] != '\0') {
        return serial_number;
    }
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
    return format_mac(mac, serial_number, sizeof(serial_number));
}

const char *pal_chip_get_hardware_version(void) {
    if (hardware_version[0] != '\0') {
        return hardware_version;
    }
    esp_chip_info_t info;
    esp_chip_info(&info);
    snprintf(hardware_version, sizeof(hardware_version), "%d", info.revision);
    return hardware_version;
}
