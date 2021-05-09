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
#include <HAP.h>

#define LUA_HAPNAME "hap"
LUAMOD_API int luaopen_hap(lua_State *L);

typedef struct {
    const HAPAccessory *primaryAccessory;
    const HAPAccessory *const *bridgedAccessories;
    bool confChanged;
} lhap_conf;

/**
 * Set HomeKit accessory server.
 */
void lhap_set_server(HAPAccessoryServerRef *server);

/**
 * Reset lhap.
 */
void lhap_reset(lua_State *L);

/**
 * Get configuration.
 */
lhap_conf lhap_get_conf(void);

/**
 * Get attribute count.
 */
size_t lhap_get_attribute_count(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void lhap_server_handle_update_state(lua_State *L, HAPAccessoryServerState state);

/**
 * Handle the session accept of the Accessory Server.
 */
void lhap_server_handle_session_accept(lua_State *L);

/**
 * Handle the session invalidate of the Accessory Server.
 */
void lhap_server_handle_session_invalidate(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LHAPLIB_H_
