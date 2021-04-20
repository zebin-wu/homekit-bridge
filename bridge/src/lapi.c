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
#include <rbtree.h>

#include <lauxlib.h>
#include <lgc.h>

#include "lapi.h"

struct lapi_callback {
    struct rb_node node; /* rbtree node */
    size_t key; /* the only key that can find the callback */
    int id; /* reference id return from luaL_ref() */
};

struct rb_root cbTree = RB_ROOT;

static const lapi_table_kv *
lapi_lookup_kv_by_name(const lapi_table_kv *kv_tab, const char *name)
{
    for (; kv_tab->key != NULL; kv_tab++) {
        if (!strcmp(kv_tab->key, name)) {
            return kv_tab;
        }
    }
    return NULL;
}

bool lapi_traverse_table(lua_State *L, int index, const lapi_table_kv *kvs, void *arg)
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
        const lapi_table_kv *kv = lapi_lookup_kv_by_name(kvs, lua_tostring(L, -1));
        // pop copy of key
        lua_pop(L, 1);
        // stack now contains: -1 => value; -2 => key; -3 => table
        if (kv) {
            if (lua_type(L, -1) != kv->type) {
                lua_pop(L, 2);
                return false;
            }
            if (kv->cb) {
                if (!kv->cb(L, kv, arg)) {
                    lua_pop(L, 2);
                    return false;
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

bool lapi_register_callback(lua_State *L, int index, size_t key)
{
    struct rb_root *root = &cbTree;
    struct rb_node **t = &(root->rb_node);
    struct rb_node *parent = NULL;

  	// Figure out where to put new node.
  	while (*t) {
        lapi_callback *this = container_of(*t, struct lapi_callback, node);
        parent = *t;
        if (key < this->key) {
            t = &((*t)->rb_left);
        } else if (key > this->key) {
            t = &((*t)->rb_right);
        } else {
            return false;
        }
  	}

    lapi_callback *new = malloc(sizeof(*new));
    if (!new) {
        return false;
    }

    lua_pushvalue(L, index);
    int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref_id == LUA_REFNIL) {
        free(new);
        return false;
    }

    new->id = ref_id;
    new->key = key;

  	// Add new node and rebalance tree.
  	rb_link_node(&new->node, parent, t);
  	rb_insert_color(&new->node, root);

    return true;
}

static lapi_callback *lapi_find_callback(struct rb_root *root, size_t key)
{
    struct rb_node *node = root->rb_node;

    while (node) {
        lapi_callback *t = container_of(node, struct lapi_callback, node);

        if (key < t->key) {
            node = node->rb_left;
        } else if (key > t->key) {
            node = node->rb_right;
        } else {
            return t;
        }
    }
    return NULL;
}

bool lapi_push_callback(lua_State *L, size_t key)
{
    lapi_callback *cb = lapi_find_callback(&cbTree, key);
    if (!cb) {
        return false;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, cb->id);
    return true;
}

bool lapi_unregister_callback(lua_State *L, size_t key)
{
    lapi_callback *cb = lapi_find_callback(&cbTree, key);
    if (!cb) {
        return false;
    }
    rb_erase(&cb->node, &cbTree);
    luaL_unref(L, LUA_REGISTRYINDEX, cb->id);
    free(cb);
    return true;
}

static void _lapi_remove_all_callbacks(lua_State *L, struct rb_node *root)
{
    if (root == NULL) {
        return;
    }
    _lapi_remove_all_callbacks(L, root->rb_left);
    _lapi_remove_all_callbacks(L, root->rb_right);
    lapi_callback *cb = container_of(root, struct lapi_callback, node);
    luaL_unref(L, LUA_REGISTRYINDEX, cb->id);
    free(cb);
}

void lapi_remove_all_callbacks(lua_State *L)
{
    _lapi_remove_all_callbacks(L, cbTree.rb_node);
}

void lapi_create_enum_table(lua_State *L, const char *enum_array[], int len)
{
    lua_createtable(L, len, 0);
    for (int i = 0; i < len; i++) {
        if (enum_array[i]) {
            lua_pushstring(L, enum_array[i]);
            lua_pushinteger(L, i);
            lua_settable(L, -3);
        }
    }
}

void lapi_collectgarbage(lua_State *L)
{
    luaC_fullgc(L, 0);
}
