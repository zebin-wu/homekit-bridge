// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_LC_H_
#define BRIDGE_SRC_LC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#define LC_TNONE            0                           // none
#define LC_TNIL             (1 << LUA_TNIL)             // nil
#define LC_TBOOLEAN         (1 << LUA_TBOOLEAN)         // boolean
#define LC_TLIGHTUSERDATA   (1 << LUA_TLIGHTUSERDATA)   // light userdata
#define LC_TNUMBER          (1 << LUA_TNUMBER)          // number
#define LC_TSTRING          (1 << LUA_TSTRING)          // string
#define LC_TTABLE           (1 << LUA_TTABLE)           // table
#define LC_TFUNCTION        (1 << LUA_TFUNCTION)        // function
#define LC_TUSERDATA        (1 << LUA_TUSERDATA)        // userdata
#define LC_TTHREAD          (1 << LUA_TTHREAD)          // thread

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
     * This function will be called when the key is parsed,
     * and the value is at the top of the stack.
     *
     * @param L 'per thread' state
     * @param arg the extra argument
     */
    bool (*cb)(lua_State *L, void *arg);
} lc_table_kv;

/**
 * Traverse Lua table.
 */
bool lc_traverse_table(lua_State *L, int idx, const lc_table_kv *kvs, void *arg);

/**
 * Traverse Lua array.
 *
 * @attention The index of the array starts from 0.
 */
bool lc_traverse_array(lua_State *L, int idx,
                        bool (*arr_cb)(lua_State *L, size_t i, void *arg),
                        void *arg);

/**
 * Get Lua main thread.
 */
lua_State *lc_getmainthread(lua_State *L);

/**
 * Collect garbage.
 */
void lc_collectgarbage(lua_State *L);

/**
 * Push traceback function to lua stack.
 */
void lc_pushtraceback(lua_State *L);

/**
 * New a coroutine.
 */
lua_State *lc_newthread(lua_State *L);

/**
 * Resume a coroutine. Must call it in protected mode.
 */
int lc_resume(lua_State *L, lua_State *from, int narg, int *nres);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_LC_H_
