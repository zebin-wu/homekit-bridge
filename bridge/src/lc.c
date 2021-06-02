// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <HAPLog.h>
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
        if (HAPStringAreEqual(kv_tab->key, key)) {
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
            lua_pushstring(L, enum_array[i]);
            lua_pushinteger(L, i);
            lua_settable(L, -3);
        }
    }
}

void lc_collectgarbage(lua_State *L) {
    luaC_fullgc(L, 0);
}

char *lc_new_str(lua_State *L, int idx) {
    size_t len;
    const char *str = lua_tolstring(L, idx, &len);
    char *copy = lc_malloc(len + 1);
    if (!copy) {
        return NULL;
    }
    HAPRawBufferCopyBytes(copy, str, len + 1);
    return copy;
}

void lc_add_searcher(lua_State *L, lua_CFunction searcher) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");

    int len = lua_rawlen(L, -1);
    lua_pushcfunction(L, searcher);
    lua_rawseti(L, -2, len);
    lua_pop(L, 2);
}
