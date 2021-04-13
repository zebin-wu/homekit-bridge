/**
 * Copyright (c) 2021 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#ifndef LUA_API_H
#define LUA_API_H

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
 * Lua userdata.
*/
typedef struct {
    const char *name;
    void *userdata;
} lapi_userdata;

/**
 * Lua callback.
*/
struct lapi_callback;
typedef struct lapi_callback lapi_callback;

/**
 * Traverse table.
*/
bool lapi_traverse_table(lua_State *L, int index, const lapi_table_kv *kvs, void *arg);

/**
 * Traverse array.
*/
bool lapi_traverse_array(lua_State *L, int index,
                         bool (*arr_cb)(lua_State *L, int i, void *arg),
                         void *arg);

/**
 * Check if the userdata is at the table.
*/
bool lapi_check_is_valid_userdata(lapi_userdata *tab, void *userdata);

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

#endif /* LUA_API_H */
