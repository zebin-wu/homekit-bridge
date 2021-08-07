// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/board.h>

const char *pal_board_get_manufacturer(void) {
    return "Unknown";
}

const char *pal_board_get_model(void) {
    return "Unknown";
}

const char *pal_board_get_serial_number(void) {
    return "Unknown";
}

const char *pal_board_get_firmware_version(void) {
#ifdef BRIDGE_VER
    return BRIDGE_VER;
#else
    return "Unknown";
#endif
}

const char *pal_board_get_hardware_version(void) {
    return "Unknown";
}
