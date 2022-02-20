// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/nvs.h>
#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPBase.h>
#include "app_int.h"
#include "lc.h"

#define LUA_NVS_HANDLE_NAME "NVS*"

typedef struct {
    pal_nvs_handle *handle;
} lnvs_handle;

static int lnvs_open(lua_State *L) {
    const char *namespace = luaL_checkstring(L, 1);

    lnvs_handle *handle = lua_newuserdata(L, sizeof(*handle));
    luaL_setmetatable(L, LUA_NVS_HANDLE_NAME);
    handle->handle = pal_nvs_open(namespace);
    if (!handle->handle) {
        luaL_error(L, "failed to open NVS handle");
    }
    return 1;
}

static lnvs_handle *lnvs_get_handle(lua_State *L, int idx) {
    lnvs_handle *handle = luaL_checkudata(L, idx, LUA_NVS_HANDLE_NAME);
    if (!handle->handle) {
        luaL_error(L, "attemp to use a closed NVS handle");
    }
    return handle;
}

static int lnvs_handle_get(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    const char *key = luaL_checkstring(L, 2);

    size_t len = pal_nvs_get_len(handle->handle, key);
    if (len == 0) {
        lua_pushnil(L);
        return 1;
    }

    luaL_Buffer B;
    luaL_buffinitsize(L, &B, len);
    luaL_addsize(&B, len);
    if (!pal_nvs_get(handle->handle, key, B.b, &B.n)) {
        luaL_error(L, "failed to get key");
    }
    luaL_pushresult(&B);
    return 1;
}

static int lnvs_handle_set(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    const char *key = luaL_checkstring(L, 2);
    size_t len;
    const char *value = luaL_checklstring(L, 3, &len);

    if (!pal_nvs_set(handle->handle, key, value, len)) {
        luaL_error(L, "failed to set key");
    }
    return 0;
}

static int lnvs_handle_remove(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    const char *key = luaL_checkstring(L, 2);

    pal_nvs_remove(handle->handle, key);
    return 0;
}

static int lnvs_handle_erase(lua_State *L) {
    pal_nvs_erase(lnvs_get_handle(L, 1)->handle);
    return 0;
}

static int lnvs_handle_commit(lua_State *L) {
    pal_nvs_commit(lnvs_get_handle(L, 1)->handle);
    return 0;
}

static int lnvs_handle_close(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);

    pal_nvs_close(handle->handle);
    handle->handle = NULL;
    return 0;
}

static int lnvs_handle_gc(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);

    if (handle->handle) {
        pal_nvs_close(handle->handle);
        handle->handle = NULL;
    }
    return 0;
}

static int lnvs_handle_tostring(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);

    if (handle->handle) {
        lua_pushfstring(L, "NVS handle (%p)", handle->handle);
    } else {
        lua_pushliteral(L, "NVS handle (closed)");
    }
    return 1;
}

static const luaL_Reg lnvs_funcs[] = {
    {"open", lnvs_open},
    {NULL, NULL},
};

/*
 * metamethods for message queue object
 */
static const luaL_Reg lnvs_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lnvs_handle_gc},
    {"__close", lnvs_handle_gc},
    {"__tostring", lnvs_handle_tostring},
    {NULL, NULL}
};

/*
 * methods for NVS handle
 */
static const luaL_Reg lnvs_handle_meth[] = {
    {"get", lnvs_handle_get},
    {"set", lnvs_handle_set},
    {"remove", lnvs_handle_remove},
    {"erase", lnvs_handle_erase},
    {"commit", lnvs_handle_commit},
    {"close", lnvs_handle_close},
    {NULL, NULL},
};

static void lnvs_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_NVS_HANDLE_NAME);  /* metatable for NVS handle */
    luaL_setfuncs(L, lnvs_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lnvs_handle_meth);  /* create method table */
    luaL_setfuncs(L, lnvs_handle_meth, 0);  /* add NVS handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_nvs(lua_State *L) {
    luaL_newlib(L, lnvs_funcs);
    lnvs_createmeta(L);
    return 1;
}
