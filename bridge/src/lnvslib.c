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
    size_t len;
    const char *namespace = luaL_checklstring(L, 1, &len);
    luaL_argcheck(L, len > 0 && len <= PAL_NVS_NAME_MAX_LEN, 1, "namespace out of range");

    lnvs_handle *handle = lua_newuserdata(L, sizeof(*handle));
    luaL_setmetatable(L, LUA_NVS_HANDLE_NAME);
    handle->handle = pal_nvs_open(namespace);
    if (luai_unlikely(!handle->handle)) {
        luaL_error(L, "failed to open NVS handle");
    }
    return 1;
}

static lnvs_handle *lnvs_get_handle(lua_State *L, int idx) {
    lnvs_handle *handle = luaL_checkudata(L, idx, LUA_NVS_HANDLE_NAME);
    if (luai_unlikely(!handle->handle)) {
        luaL_error(L, "attemp to use a closed NVS handle");
    }
    return handle;
}

static int lnvs_handle_get(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    size_t len;
    const char *key = luaL_checklstring(L, 2, &len);
    luaL_argcheck(L, len > 0 && len <= PAL_NVS_KEY_MAX_LEN, 2, "key out of range");

    len = pal_nvs_get_len(handle->handle, key);
    if (len == 0) {
        lua_pushnil(L);
        return 1;
    }

    lua_getfield(L, lua_upvalueindex(1), "decode");

    luaL_Buffer B;
    luaL_buffinitsize(L, &B, len);
    luaL_addsize(&B, len);
    if (luai_unlikely(!pal_nvs_get(handle->handle, key, B.b, len))) {
        luaL_error(L, "failed to get key");
    }
    luaL_pushresult(&B);

    // return json.decode(s)
    lua_call(L, 1, 1);
    return 1;
}

static int lnvs_handle_set(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    size_t len;
    const char *key = luaL_checklstring(L, 2, &len);
    luaL_argcheck(L, len > 0 && len <= PAL_NVS_KEY_MAX_LEN, 2, "key out of range");

    if (lua_type(L, 3) == LUA_TNIL) {
        pal_nvs_remove(handle->handle, key);
        return 0;
    }
    // json.encode(value)
    lua_getfield(L, lua_upvalueindex(1), "encode");
    lua_insert(L, 3);
    lua_call(L, 1, 1);

    const char *value = luaL_checklstring(L, 3, &len);

    if (luai_unlikely(!pal_nvs_set(handle->handle, key, value, len))) {
        luaL_error(L, "failed to set key");
    }
    return 0;
}

static int lnvs_handle_erase(lua_State *L) {
    if (!pal_nvs_erase(lnvs_get_handle(L, 1)->handle)) {
        luaL_error(L, "failed to erase all");
    }
    return 0;
}

static int lnvs_handle_commit(lua_State *L) {
    if (luai_unlikely(!pal_nvs_commit(lnvs_get_handle(L, 1)->handle))) {
        luaL_error(L, "failed to commit all changes");
    }
    return 0;
}

static int lnvs_handle_close(lua_State *L) {
    lnvs_handle *handle = lnvs_get_handle(L, 1);
    pal_nvs_close(handle->handle);
    handle->handle = NULL;
    return 0;
}

static int lnvs_handle_gc(lua_State *L) {
    lnvs_handle *handle = luaL_checkudata(L, 1, LUA_NVS_HANDLE_NAME);
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
    {"erase", lnvs_handle_erase},
    {"commit", lnvs_handle_commit},
    {"close", lnvs_handle_close},
    {NULL, NULL},
};

static void lnvs_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_NVS_HANDLE_NAME);  /* metatable for NVS handle */
    luaL_setfuncs(L, lnvs_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lnvs_handle_meth);  /* create method table */
    lua_getglobal(L, "cjson");
    luaL_setfuncs(L, lnvs_handle_meth, 1);  /* add NVS handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_nvs(lua_State *L) {
    luaL_newlib(L, lnvs_funcs);
    lnvs_createmeta(L);
    return 1;
}
