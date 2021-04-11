/**
 * Copyright (c) 2021 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include <platform/board.h>
#include <esp_system.h>
#include <esp_ota_ops.h>

#define SERIAL_NUMBER_BUF_LEN (6 * 2 + 1)
#define HARDWARE_VERSION_BUF_LEN 16

static char serial_number[SERIAL_NUMBER_BUF_LEN];
static char hardware_version[HARDWARE_VERSION_BUF_LEN];

/*
 * Format 48-bit MAC address into the user-supplied buffer.
 * Buffer should be at least 13 bytes long.
 * String will be NUL-terminated and truncated if necessary.
 */
static char *format_mac(const uint8_t *mac, char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
	    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return buf;
}

const char *pfm_board_get_manufacturer(void)
{
    return "Espressif";
}

const char *pfm_board_get_model(void)
{
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

const char *pfm_board_get_serial_number(void)
{
    if (serial_number[0] != '\0') {
        return serial_number;
    }
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
    return format_mac(mac, serial_number, sizeof(serial_number));
}

const char *pfm_board_get_firmware_version(void)
{
    return esp_ota_get_app_description()->version;
}

const char *pfm_board_get_hardware_version(void)
{
    if (hardware_version[0] != '\0') {
        return hardware_version;
    }
    esp_chip_info_t info;
    esp_chip_info(&info);
    snprintf(hardware_version, sizeof(hardware_version), "%d", info.revision);
    return hardware_version;
}
