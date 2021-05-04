// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <rbtree.h>

#include <HAPLog.h>
#include <lauxlib.h>
#include <lgc.h>

#include "AppInt.h"
#include "lc.h"

struct lc_callback {
    struct rb_node node; /* rbtree node */
    size_t key; /* the only key that can find the callback */
    int id; /* reference id return from luaL_ref() */
};

static const HAPLogObject lc_log = {
    .subsystem = kHAPApplication_LogSubsystem,
    .category = "lc",
};

struct rb_root cbTree = RB_ROOT;

static const lc_table_kv *
lc_lookup_kv_by_name(const lc_table_kv *kv_tab, const char *key)
{
    for (; kv_tab->key != NULL; kv_tab++) {
        if (!strcmp(kv_tab->key, key)) {
            return kv_tab;
        }
    }
    return NULL;
}

bool lc_traverse_table(lua_State *L, int idx, const lc_table_kv *kvs, void *arg)
{
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
            int type = (1 >> lua_type(L, -1));
            if (type & kv->type) {
                HAPLogError(&lc_log, "%s: invalid type: %d", __func__, type);
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
                         bool (*arr_cb)(lua_State *L, int i, void *arg),
                         void *arg)
{
    if (!arr_cb) {
        return false;
    }

    lua_pushvalue(L, idx);
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

bool lc_register_callback(lua_State *L, int idx, size_t key)
{
    struct rb_root *root = &cbTree;
    struct rb_node **t = &(root->rb_node);
    struct rb_node *parent = NULL;

  	// Figure out where to put new node.
  	while (*t) {
        lc_callback *this = container_of(*t, struct lc_callback, node);
        parent = *t;
        if (key < this->key) {
            t = &((*t)->rb_left);
        } else if (key > this->key) {
            t = &((*t)->rb_right);
        } else {
            return false;
        }
  	}

    lc_callback *new = lc_malloc(sizeof(*new));
    if (!new) {
        return false;
    }

    lua_pushvalue(L, idx);
    int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref_id == LUA_REFNIL) {
        lc_free(new);
        return false;
    }

    new->id = ref_id;
    new->key = key;

  	// Add new node and rebalance tree.
  	rb_link_node(&new->node, parent, t);
  	rb_insert_color(&new->node, root);

    return true;
}

static lc_callback *lc_find_callback(struct rb_root *root, size_t key)
{
    struct rb_node *node = root->rb_node;

    while (node) {
        lc_callback *t = container_of(node, struct lc_callback, node);

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

bool lc_push_callback(lua_State *L, size_t key)
{
    lc_callback *cb = lc_find_callback(&cbTree, key);
    if (!cb) {
        return false;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, cb->id);
    return true;
}

bool lc_unregister_callback(lua_State *L, size_t key)
{
    lc_callback *cb = lc_find_callback(&cbTree, key);
    if (!cb) {
        return false;
    }
    rb_erase(&cb->node, &cbTree);
    luaL_unref(L, LUA_REGISTRYINDEX, cb->id);
    lc_free(cb);
    return true;
}

static void _lc_remove_all_callbacks(lua_State *L, struct rb_node *root)
{
    if (root == NULL) {
        return;
    }
    _lc_remove_all_callbacks(L, root->rb_left);
    _lc_remove_all_callbacks(L, root->rb_right);
    lc_callback *cb = container_of(root, struct lc_callback, node);
    luaL_unref(L, LUA_REGISTRYINDEX, cb->id);
    lc_free(cb);
}

void lc_remove_all_callbacks(lua_State *L)
{
    _lc_remove_all_callbacks(L, cbTree.rb_node);
}

void lc_create_enum_table(lua_State *L, const char *enum_array[], int len)
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

void lc_collectgarbage(lua_State *L)
{
    luaC_fullgc(L, 0);
}

char *lc_new_str(lua_State *L, int idx)
{
    size_t len;
    const char *str = lua_tolstring(L, idx, &len);
    char *copy = lc_malloc(len + 1);
    return copy ? strcpy(copy, str) : NULL;
}
