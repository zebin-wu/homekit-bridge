// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef LHAPLIB_H
#define LHAPLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <HAP.h>

#define LUA_HAPNAME "hap"
LUAMOD_API int luaopen_hap(lua_State *L);

/**
 * Get accessory.
 */
const HAPAccessory *lhap_get_accessory(void);

/**
 * Get bridged accessories.
 */
const HAPAccessory *const *lhap_get_bridged_accessories(void);

/**
 * Get attribute count.
 */
size_t lhap_get_attribute_count(void);

/**
 * De-initialize lhap.
 */
void lhap_deinitialize();

#ifdef __cplusplus
}
#endif

#endif /* LHAPLIB_H */
