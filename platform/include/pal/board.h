// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_BOARD_H_
#define PLATFORM_INCLUDE_PAL_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get board information.
 * Every platform must to implement it.
 */

/**
 * Get manufacturer.
 */
const char *pal_board_get_manufacturer(void);

/**
 * Get model.
 */
const char *pal_board_get_model(void);

/**
 * Get serial number.
 */
const char *pal_board_get_serial_number(void);

/**
 * Get firmware version.
 */
const char *pal_board_get_firmware_version(void);

/**
 * Get hardware version.
 */
const char *pal_board_get_hardware_version(void);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_BOARD_H_
