// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <lauxlib.h>
#include <lgc.h>

#include "app_int.h"
#include "lc.h"

static const HAPLogObject lc_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lc",
};

static const lc_table_kv *
lc_lookup_kv_by_name(const lc_table_kv *kv_tab, const char *key) {
    for (; kv_tab->key != NULL; kv_tab++) {
        if (!strcmp(kv_tab->key, key)) {
            return kv_tab;
        }
    }
    return NULL;
}

bool lc_traverse_table(lua_State *L, int idx, const lc_table_kv *kvs, void *arg) {
    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, idx);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2)) {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        const char *key = lua_tostring(L, -1);
        const lc_table_kv *kv = lc_lookup_kv_by_name(kvs, key);
        // pop copy of key
        lua_pop(L, 1);
        // stack now contains: -1 => value; -2 => key; -3 => table
        if (kv) {
            if (!((1 << lua_type(L, -1)) & kv->type)) {
                HAPLogError(&lc_log, "%s: Invalid type: %s", __func__, kv->key);
                lua_pop(L, 2);
                return false;
            }
            if (kv->cb) {
                if (!kv->cb(L, kv, arg)) {
                    lua_pop(L, 2);
                    return false;
                }
            }
        } else {
            HAPLogError(&lc_log, "%s: Unknown key \"%s\".", __func__, key);
            lua_pop(L, 2);
            return false;
        }
        // pop value, leaving original key
        lua_pop(L, 1);
        // stack now contains: -1 => key; -2 => table
    }
    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
    return true;
}

bool lc_traverse_array(lua_State *L, int idx,
        bool (*arr_cb)(lua_State *L, size_t i, void *arg), void *arg) {
    if (!arr_cb) {
        return false;
    }
    lua_pushvalue(L, idx);
    lua_pushnil(L);
    for (size_t i = 0; lua_next(L, -2); lua_pop(L, 1), i++) {
        if (!lua_isinteger(L, -2)) {
            goto invalid_array;
        }
        int isnum;
        lua_Integer num = lua_tointegerx(L, -2, &isnum);
        if (!isnum || num != i + 1) {
            goto invalid_array;
        }
        if (!arr_cb(L, i, arg)) {
            goto err;
        }
    }
    lua_pop(L, 1);
    return true;

invalid_array:
    HAPLogError(&lc_log, "%s: Invalid array.", __func__);
err:
    lua_pop(L, 2);
    return false;
}

void lc_create_enum_table(lua_State *L, const char *enum_array[], int len) {
    lua_createtable(L, len, 0);
    for (int i = 0; i < len; i++) {
        if (enum_array[i]) {
            lua_pushinteger(L, i);
            lua_setfield(L, -2, enum_array[i]);
        }
    }
}

void lc_collectgarbage(lua_State *L) {
    luaC_fullgc(L, 0);
}

void lc_add_searcher(lua_State *L, lua_CFunction searcher) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");

    int len = lua_rawlen(L, -1);
    lua_pushcfunction(L, searcher);
    lua_rawseti(L, -2, len + 1);
    lua_pop(L, 2);
}

void lc_set_path(lua_State *L, const char *path) {
    lua_getglobal(L, "package");

    lua_pushstring(L, path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

void lc_set_cpath(lua_State *L, const char *cpath) {
    lua_getglobal(L, "package");

    lua_pushstring(L, cpath);
    lua_setfield(L, -2, "cpath");
    lua_pop(L, 1);
}

static int traceback(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg) {
        luaL_traceback(L, L, msg, 1);
    } else {
        lua_pushliteral(L, "(no error message)");
    }
    return 1;
}

void lc_push_traceback(lua_State *L) {
    lua_pushcfunction(L, traceback);
}

lua_State *lc_newthread(lua_State *L) {
    lua_State *co = lua_newthread(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, co);
    return co;
}

int lc_freethread(lua_State *L) {
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, L);
    return lua_resetthread(L);
}
