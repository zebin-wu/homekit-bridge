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

#include <lua.h>
#include <platform/memory.h>

#define lc_malloc(size)     pal_mem_alloc(size)
#define lc_calloc(size)     pal_mem_calloc(size)
#define lc_free(p)          pal_mem_free(p)
#define lc_safe_free(p)     do { if (p) { lc_free((void *)p); (p) = NULL; } } while (0)

#define LC_TNONE            0                           // none
#define LC_TNIL             (1 >> LUA_TNIL)             // nil
#define LC_TBOOLEAN         (1 >> LUA_TBOOLEAN)         // boolean
#define LC_TLIGHTUSERDATA   (1 >> LUA_TLIGHTUSERDATA)   // light userdata
#define LC_TNUMBER          (1 >> LUA_TNUMBER)          // number
#define LC_TSTRING          (1 >> LUA_TSTRING)          // string
#define LC_TTABLE           (1 >> LUA_TTABLE)           // table
#define LC_TFUNCTION        (1 >> LUA_TFUNCTION)        // function
#define LC_TUSERDATA        (1 >> LUA_TUSERDATA)        // userdata
#define LC_TTHREAD          (1 >> LUA_TTHREAD)          // thread

// any
#define LC_TANY             (LC_TNIL | LC_TBOOLEAN | LC_TLIGHTUSERDATA | LC_TNUMBER | \
    LC_TSTRING | LC_TTABLE | LC_TFUNCTION | LC_TUSERDATA | LC_TTHREAD)

/**
 * Lua table key-value.
 */
typedef struct lc_table_kv {
    const char *key;    /* key */
    /**
     * Lua type. View macros starting with "LC_T", each type will occupy 1 bit.
     */
    uint32_t type;
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
