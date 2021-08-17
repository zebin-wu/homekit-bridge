// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_LHAPLIB_H_
#define BRIDGE_SRC_LHAPLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LUA_HAP_NAME "hap"
LUAMOD_API int luaopen_hap(lua_State *L);

/**
 * Set HomeKit platform.
 */
void lhap_set_platform(HAPPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LHAPLIB_H_
