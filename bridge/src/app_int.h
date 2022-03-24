// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
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

#define LUA_CHIP_NAME "chip"
LUAMOD_API int luaopen_chip(lua_State *L);

#define LUA_CIPHER_NAME "cipher"
LUAMOD_API int luaopen_cipher(lua_State *L);

#define LUA_HAP_NAME "hap"
LUAMOD_API int luaopen_hap(lua_State *L);

#define LUA_HASH_NAME "hash"
LUAMOD_API int luaopen_hash(lua_State *L);

#define LUA_LOG_NAME "log"
LUAMOD_API int luaopen_log(lua_State *L);

#define LUA_CORE_NAME "core"
LUAMOD_API int luaopen_core(lua_State *L);

#define LUA_SOCKET_NAME "socket"
LUAMOD_API int luaopen_socket(lua_State *L);

#define LUA_MQ_NAME "mq"
LUAMOD_API int luaopen_mq(lua_State *L);

#define LUA_SSL_NAME "ssl"
LUAMOD_API int luaopen_ssl(lua_State *L);

#define LUA_DNS_NAME "dns"
LUAMOD_API int luaopen_dns(lua_State *L);

#define LUA_NVS_NAME "nvs"
LUAMOD_API int luaopen_nvs(lua_State *L);

#define LUA_STREAM_NAME "stream"
LUAMOD_API int luaopen_stream(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_APP_INT_H_
