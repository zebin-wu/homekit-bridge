// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef LUA_COMMON_H
#define LUA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <lua.h>
#include <platform/memory.h>

#define lc_malloc(size)     pal_mem_alloc(size)
#define lc_calloc(n, size)  pal_mem_calloc(n, size)
#define lc_free(p)          pal_mem_free(p)
#define lc_safe_free(p)     do { if (p) { lc_free((void *)p); (p) = NULL; } } while (0)

/**
 * Lua table key-value.
 */
typedef struct lc_table_kv {
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
    bool (*cb)(lua_State *L, const struct lc_table_kv *kv, void *arg);
} lc_table_kv;

/**
 * Lua callback.
 */
struct lc_callback;
typedef struct lc_callback lc_callback;

/**
 * Traverse Lua table.
 */
bool lc_traverse_table(lua_State *L, int idx, const lc_table_kv *kvs, void *arg);

/**
 * Traverse Lua array.
 */
bool lc_traverse_array(lua_State *L, int idx,
                        bool (*arr_cb)(lua_State *L, int i, void *arg),
                        void *arg);

/**
 * Register callback.
 */
bool lc_register_callback(lua_State *L, int idx, size_t key);

/**
 * Unregister callback.
 */
bool lc_unregister_callback(lua_State *L, size_t key);

/**
 * Push callback to Lua stack.
 */
bool lc_push_callback(lua_State *L, size_t key);

/**
 * Remove all callbacks.
 */
void lc_remove_all_callbacks(lua_State *L);

/**
 * Create a enum table.
 */
void lc_create_enum_table(lua_State *L, const char *enum_array[], int len);

/**
 * Collect garbage.
 */
void lc_collectgarbage(lua_State *L);

/**
 * Return a new copy from the str on the "idx" of the stack.
 */
char *lc_new_str(lua_State *L, int idx);

#ifdef __cplusplus
}
#endif

#endif /* LUA_COMMON_H */
