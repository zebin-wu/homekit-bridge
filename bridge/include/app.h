// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_INCLUDE_APP_H_
#define BRIDGE_INCLUDE_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAP.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Default lua entry script name.
#define BRIDGE_LUA_ENTRY_DEFAULT "main"

/**
 * Run the application lua entry.
 *
 * @param dir the path of the scripts directory.
 * @param entry the name of the entry script.
 *
 * @return true on success.
 * @return false on failure.
 */
bool app_lua_run(const char *dir, const char *entry);

/**
 * Close the lua state.
 */
void app_lua_close(void);

/**
 * Initialize App.
 */
void app_init(HAPPlatform *platform);

/**
 * De-initialize App.
 */
void app_deinit();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_INCLUDE_APP_H_
