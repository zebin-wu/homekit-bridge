// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_APP_INT_H_
#define BRIDGE_SRC_APP_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

/**
 * Log subsystem used by the HAP Bridge implementation.
 */
#define APP_BRIDGE_LOG_SUBSYSTEM "com.apple.mfi.HomeKit.Bridge"

/**
 * Application context. Will be passed to callbacks.
 */
typedef struct {
    lua_State *L;
} app_context;

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_APP_INT_H_
