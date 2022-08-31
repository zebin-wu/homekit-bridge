// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/cli.h>
#include <HAPLog.h>
#include "lc.h"
#include "app_int.h"

#define LCLI_CMD_TABLE "_CMDTABLE"

static const HAPLogObject lcli_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "cli",
};

static int lcli_prun(lua_State *L) {
    int argc = lua_tointeger(L, -2);
    char **argv = lua_touserdata(L, -1);
    lua_pop(L, 2);

    lua_State *co = lc_newthread(L);

    if (luai_unlikely(!luaL_getsubtable(co, LUA_REGISTRYINDEX, LCLI_CMD_TABLE))) {
        HAPFatalError();
    }

    if (luai_unlikely(lua_getfield(co, -1, argv[0]) != LUA_TTABLE)) {
        luaL_error(L, "cmd '%s' not found in registry._CMDTABLE", argv[0]);
    }
    lua_remove(co, -2);

    if (luai_unlikely(lua_geti(co, -1, 3) != LUA_TFUNCTION)) {
        luaL_error(L, "registry._CMDTABLE['%s'][3] not a function", argv[0]);
    }
    lua_remove(co, -2);

    if (luai_unlikely(!lua_checkstack(co, argc - 1))) {
        luaL_error(L, "stack overflow");
    }
    for (int i = 1; i < argc; i++) {
        lua_pushstring(co, argv[i]);
    }

    int nres, status;
    status = lc_resume(co, L, argc - 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        lua_error(L);
    }
    return 0;
}

static int lcli_run(int argc, char *argv[], void *ctx) {
    int ret = -1;
    lua_State *co = ctx;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lcli_prun);
    lua_pushinteger(L, argc);
    lua_pushlightuserdata(L, argv);

    int status = lua_pcall(L, 2, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&lcli_log, "%s: %s", __func__, lua_tostring(L, -1));
        goto end;
    }

    ret = 0;

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return ret;
}

static int lcli_register(lua_State *L) {
    const char *cmd = luaL_checkstring(L, 1);
    const char *help = luaL_optstring(L, 2, NULL);
    const char *hint = luaL_optstring(L, 3, NULL);
    luaL_checktype(L, 4, LUA_TFUNCTION);

    pal_err err = pal_cli_register(&(pal_cli_info) {
        .cmd = cmd,
        .help = help,
        .hint = hint,
        .func = lcli_run,
    }, L);
    switch (err) {
    case PAL_ERR_OK:
        /* registry._CMDTABLE = { help, hint, func } */
        lua_pushvalue(L, 1);
        lua_createtable(L, 3, 0);
        for (int i = 2; i <= 4; i++) {
            lua_pushvalue(L, i);
            lua_seti(L, -2, i - 1);
        }
        lua_settable(L, lua_upvalueindex(1));
        break;
    default:
        luaL_error(L, pal_err_string(err));
        break;
    }
    return 0;
}

static const luaL_Reg clilib[] = {
    {"register", lcli_register},
    {NULL, NULL},
};

LUAMOD_API int luaopen_cli(lua_State *L) {
    luaL_checkversion(L);
    luaL_newlibtable(L, clilib);
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LCLI_CMD_TABLE);
    luaL_setfuncs(L, clilib, 1);
    return 1;
}
