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
#include <HAP.h>

/**
 * Log subsystem used by the HAP Bridge implementation.
 */
#define APP_BRIDGE_LOG_SUBSYSTEM "com.apple.mfi.HomeKit.Bridge"

#define LUA_BOARD_NAME "board"
LUAMOD_API int luaopen_board(lua_State *L);

#define LUA_CIPHER_NAME "cipher"
LUAMOD_API int luaopen_cipher(lua_State *L);

#define LUA_HAP_NAME "hap"
LUAMOD_API int luaopen_hap(lua_State *L);

#define LUA_HASH_NAME "hash"
LUAMOD_API int luaopen_hash(lua_State *L);

#define LUA_LOG_NAME "log"
LUAMOD_API int luaopen_log(lua_State *L);

#define LUA_UDP_NAME "udp"
LUAMOD_API int luaopen_udp(lua_State *L);

#define LUA_TIME_NAME "time"
LUAMOD_API int luaopen_time(lua_State *L);

#define LUA_SOCKET_NAME "socket"
LUAMOD_API int luaopen_socket(lua_State *L);

/**
 * Set HomeKit platform.
 */
void lhap_set_platform(HAPPlatform *platform);

/**
 * Get Lua main thread.
 */
lua_State *app_get_lua_main_thread();

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_APP_INT_H_
