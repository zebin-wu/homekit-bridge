// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_LNETLIB_H_
#define BRIDGE_SRC_LNETLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LUA_NET_UDP_NAME "net.udp"
LUAMOD_API int luaopen_net_udp(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LNETLIB_H_
