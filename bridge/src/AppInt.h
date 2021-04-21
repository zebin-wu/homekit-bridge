// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef APP_INT_H
#define APP_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

/**
 * Log subsystem used by the HAP Application implementation.
 */
#define kHAPApplication_LogSubsystem "com.apple.mfi.HomeKit.Application"

/**
 * Accessory context. Will be passed to callbacks.
 */
typedef struct {
    lua_State *L;
} AccessoryContext;

#ifdef __cplusplus
}
#endif

#endif /* APP_INT_H */
