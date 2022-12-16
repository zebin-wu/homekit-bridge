// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_ESP_INCLUDE_PAL_CLI_INT_H_
#define PLATFORM_ESP_INCLUDE_PAL_CLI_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize CLI module.
 */
void pal_cli_init();

/**
 * De-initialize CLI module.
 */
void pal_cli_deinit();

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_ESP_INCLUDE_PAL_CLI_INT_H_
