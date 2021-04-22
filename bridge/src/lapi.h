// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef COMMON_LUA_API_H
#define COMMON_LUA_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <lua.h>

/**
 * Lua table key-value.
 */
typedef struct lapi_table_kv {
    const char *key;    /* key */
    int type;   /* value type */
    /**
     * This callbacktion will be called when the key is parsed,
     * and the value is at the top of the stack.
     *
     * @param L 'per thread' state
     * @param kv the point to current key-value
     * @param arg the extra argument
     */
    bool (*cb)(lua_State *L, const struct lapi_table_kv *kv, void *arg);
} lapi_table_kv;

/**
 * Lua callback.
 */
struct lapi_callback;
typedef struct lapi_callback lapi_callback;

/**
 * Traverse Lua table.
 */
bool lapi_traverse_table(lua_State *L, int index, const lapi_table_kv *kvs, void *arg);

/**
 * Traverse Lua array.
 */
bool lapi_traverse_array(lua_State *L, int index,
                         bool (*arr_cb)(lua_State *L, int i, void *arg),
                         void *arg);

/**
 * Register callback.
 */
bool lapi_register_callback(lua_State *L, int index, size_t key);

/**
 * Unregister callback.
 */
bool lapi_unregister_callback(lua_State *L, size_t key);

/**
 * Push callback to Lua stack.
 */
bool lapi_push_callback(lua_State *L, size_t key);

/**
 * Remove all callbacks.
 */
void lapi_remove_all_callbacks(lua_State *L);

/**
 * Create a enum table.
 */
void lapi_create_enum_table(lua_State *L, const char *enum_array[], int len);

/**
 * Collect garbage.
 */
void lapi_collectgarbage(lua_State *L);

/**
 * Return a new string copy from str.
 */
char *lapi_new_str(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_LUA_API_H */
