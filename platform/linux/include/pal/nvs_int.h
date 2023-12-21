// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NVS_INT_H_
#define PLATFORM_INCLUDE_PAL_NVS_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize NVS module.
 *
 * @param dir A directory to store nvs.
 */
void pal_nvs_init(const char *dir);

/**
 * De-initialize NVS module.
 */
void pal_nvs_deinit();

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NVS_INT_H_
