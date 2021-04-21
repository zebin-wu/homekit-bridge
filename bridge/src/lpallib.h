// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef LPALLIB_H
#define LPALLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LUA_PALNAME "pal"
LUAMOD_API int luaopen_pal(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* LPALLIB_H */
