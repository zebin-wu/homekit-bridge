// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_LPALLIB_H_
#define BRIDGE_SRC_LPALLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LUA_PAL_BOARD_NAME "pal.board"
LUAMOD_API int luaopen_pal_board(lua_State *L);

#define LUA_PAL_NET_UDP_NAME "pal.net.udp"
LUAMOD_API int luaopen_pal_net_udp(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LPALLIB_H_
