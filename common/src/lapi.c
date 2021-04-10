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
#include <stdlib.h>
#include <string.h>

#include <common/lapi.h>
#include <lauxlib.h>

struct lapi_callback {
    int type;
    void *parent;
    int id; /* ref id return from luaL_ref() */
    struct lapi_callback *next;
};

lapi_table_kv *lapi_lookup_kv_by_name(lapi_table_kv *kv_tab, const char *name)
{
    for (;kv_tab->key != NULL; kv_tab++) {
        if (!strcmp(kv_tab->key, name)) {
            return kv_tab;
        }
    }
    return NULL;
}

bool lapi_traverse_table(lua_State *L, int index, lapi_table_kv *kvs, void *arg)
{
    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2)) {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        lapi_table_kv *kv = lapi_lookup_kv_by_name(kvs, lua_tostring(L, -1));
        // pop copy of key
        lua_pop(L, 1);
        // stack now contains: -1 => value; -2 => key; -3 => table
        if (kv) {
            if (lua_type(L, -1) != kv->type) {
                lua_pop(L, 2);
                return false;
            }
            if (kv->cb) {
                if (kv->cb(L, kv, arg) == false) {
                    lua_pop(L, 2);
                    return false;
                } else {
                }
            }
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

bool lapi_traverse_array(lua_State *L, int index,
                         bool (*arr_cb)(lua_State *L, int i, void *arg),
                         void *arg)
{
    if (!arr_cb) {
        return false;
    }

    lua_pushvalue(L, index);
    lua_pushnil(L);
    for (int i = 0; lua_next(L, -2); lua_pop(L, 1), i++) {
        if (!arr_cb(L, i, arg)) {
            lua_pop(L, 2);
            return false;
        }
    }
    lua_pop(L, 1);
    return true;
}

bool lapi_check_is_valid_userdata(lapi_userdata *tab, void *userdata)
{
    for (;tab->userdata != NULL; tab++) {
        if (userdata == tab->userdata) {
            return true;
        }
    }
    return false;
}

bool lapi_register_callback(lapi_callback **head, lua_State *L,
                            int index, void *parent, int type)
{
    lapi_callback **t = head;
    lapi_callback *new;

    lua_pushvalue(L, index);
    int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref_id == LUA_REFNIL) {
        return false;
    }

    while (*t != NULL) {
        if ((*t)->id == ref_id) {
            return false;
        }
        t = &(*t)->next;
    }
    new = malloc(sizeof(*new));
    if (!new) {
        return false;
    }
    new->id = ref_id;
    new->type = type;
    new->parent = parent;
    new->next = NULL;
    *t = new;
    return true;
}

bool lapi_push_callback(lapi_callback *head, lua_State *L,
                        void *parent, int type)
{
    lapi_callback *t = head;
    while (t != NULL) {
        if (t->parent == parent && t->type == type) {
            break;
        }
        t = t->next;
    }
    if (!t) {
        return false;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, t->id);
    return true;
}

void lapi_del_callback_list(lapi_callback **head, lua_State *L)
{
    lapi_callback *t;
    while (*head) {
        t = *head;
        luaL_unref(L, LUA_REGISTRYINDEX, t->id);
        free(t);
        head = &(*head)->next;
    }
    *head = NULL;
}

void lapi_create_enum_table(lua_State *L, const char *enum_array[], int len)
{
    lua_newtable(L);
    for (int i = 0; i < len; i++) {
        if (enum_array[i]) {
            lua_pushstring(L, enum_array[i]);
            lua_pushnumber(L, i);
            lua_settable(L, -3);
        }
    }
}
