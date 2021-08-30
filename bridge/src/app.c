// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <lualib.h>

#include <app.h>

#include "app_int.h"
#include "lc.h"

// Declare the function of lua-cjson.
#define LUA_CJSON_NAME "cjson"
extern int luaopen_cjson(lua_State *L);

static lua_State *L;

static const luaL_Reg globallibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_IOLIBNAME, luaopen_io},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_LOG_NAME, luaopen_log},
    {NULL, NULL}
};

static const luaL_Reg dynamiclibs[] = {
    {LUA_HAP_NAME, luaopen_hap},
    {LUA_BOARD_NAME, luaopen_board},
    {LUA_NET_UDP_NAME, luaopen_net_udp},
    {LUA_TIMER_NAME, luaopen_timer},
    {LUA_HASH_NAME, luaopen_hash},
    {LUA_CIPHER_NAME, luaopen_cipher},
    {LUA_CJSON_NAME, luaopen_cjson},
    {NULL, NULL}
};

static int searcher_dl(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const luaL_Reg *lib = dynamiclibs;

    for (; lib->func; lib++) {
        if (HAPStringAreEqual(lib->name, name)) {
            break;
        }
    }
    if (lib->func) {
        lua_pushcfunction(L, lib->func);
    } else {
        lua_pushfstring(L, "no module '%s' in dynamiclibs", name);
    }
    return 1;
}

bool app_lua_run(const char *dir, const char *entry) {
    HAPPrecondition(dir);
    HAPPrecondition(entry);

    char path[256];

    L  = luaL_newstate();
    if (L == NULL) {
        HAPLogError(&kHAPLog_Default, "Cannot create state: not enough memory");
        goto err;
    }

    // load global libraries
    for (const luaL_Reg *lib = globallibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // set file path
    HAPAssert(HAPStringWithFormat(path, sizeof(path), "%s/?.lua;%s/?.luac", dir, dir) == kHAPError_None);
    lc_set_path(L, path);

    // set C path
    lc_set_cpath(L, "");

    // add searcher_dl to package.searcher
    lc_add_searcher(L, searcher_dl);

    // run entry scripts
    HAPAssert(HAPStringWithFormat(path, sizeof(path), "%s/%s.luac", dir, entry) == kHAPError_None);
    int status = luaL_dofile(L, path);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        HAPLogError(&kHAPLog_Default, "%s.luac: %s", entry, msg);
        goto err1;
    }

    if (!lua_isboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default,
            "%s.luac returned is not a boolean.", entry);
        goto err1;
    }

    if (!lua_toboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default, "Failed to configure.");
        goto err1;
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
    return true;

err1:
    lua_settop(L, 0);
    lc_collectgarbage(L);
err:
    return false;
}

void app_lua_close(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

void app_init(HAPPlatform *platform) {
    lhap_set_platform(platform);
}

void app_deinit() {
    /*no-op*/
}
