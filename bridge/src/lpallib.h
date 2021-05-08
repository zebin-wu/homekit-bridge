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

#define LUA_PALNAME "pal"
LUAMOD_API int luaopen_pal(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LPALLIB_H_
