// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
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

void lc_collectgarbage(lua_State *L) {
    luaC_fullgc(L, 0);
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

void lc_pushtraceback(lua_State *L) {
    lua_pushcfunction(L, traceback);
}

int lc_resume(lua_State *L, lua_State *from, int narg, int *nres) {
    int before_status = lua_status(L);
    if (luai_unlikely(before_status != LUA_OK && before_status != LUA_YIELD)) {
        luaL_error(L, "invalid coroutine status");
    }

    int status = lua_resume(L, from, narg, nres);
    switch (status) {
    case LUA_OK:
        if (luai_unlikely(!lua_checkstack(L, *nres))) {
            luaL_error(L, "too many arguments to resume");
        }
        lua_xmove(L, from, *nres);
        if (before_status == LUA_YIELD) {
            lua_pushnil(L);
            lua_rawsetp(L, LUA_REGISTRYINDEX, L);
        }
        lua_resetthread(L);
        break;
    case LUA_YIELD:
        if (before_status == LUA_OK) {
            lua_pushthread(L);
            lua_rawsetp(L, LUA_REGISTRYINDEX, L);
        }
        break;
    default:
        luaL_traceback(from, L, lua_tostring(L, -1), 1);
        if (before_status == LUA_YIELD) {
            lua_pushnil(L);
            lua_rawsetp(L, LUA_REGISTRYINDEX, L);
        }
        lua_resetthread(L);
        break;
    }
    return status;
}
