// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef LLOGLIB_H
#define LLOGLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LUA_LOGNAME "log"
LUAMOD_API int luaopen_log(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* LLOGLIB_H */
