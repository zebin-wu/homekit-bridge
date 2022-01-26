// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CHIP_H_
#define PLATFORM_INCLUDE_PAL_CHIP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get manufacturer.
 */
const char *pal_chip_get_manufacturer(void);

/**
 * Get model.
 */
const char *pal_chip_get_model(void);

/**
 * Get serial number.
 */
const char *pal_chip_get_serial_number(void);

/**
 * Get hardware version.
 */
const char *pal_chip_get_hardware_version(void);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CHIP_H_
