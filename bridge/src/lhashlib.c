// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <HAPBase.h>

#include "md5.h"
#include "lhashlib.h"

#define LUA_HASH_OBJ_NAME "HashObject"

#define LHASH_GET_OBJ(L, idx) \
    luaL_checkudata(L, idx, LUA_HASH_OBJ_NAME)

typedef struct {
    const char *name;
    size_t ctx_len; /* The length of the context. */
    size_t digest_len;  /* The length of the digest. */
    void (*init)(void *ctx); /* Initialize context. */
    bool (*update)(void *ctx, const void *data, size_t len); /* Update data. */
    void (*digest)(void *ctx, void *output); /* Get the digest. */
} lhash_method;

static void lhash_md5_init(void *ctx) {
    md5_init((md5_ctx *)ctx);
}

bool lhash_md5_update(void *ctx, const void *data, size_t len) {
    return md5_update((md5_ctx *)ctx, data, len);
}

void lhash_md5_digest(void *ctx, void *output) {
    md5_digest(ctx, output);
}

static const lhash_method lhash_mths[] = {
    {
        .name = "md5",
        .ctx_len = sizeof(md5_ctx),
        .digest_len = MD5_HASHSIZE,
        .init = lhash_md5_init,
        .update = lhash_md5_update,
        .digest = lhash_md5_digest,
    },
};

/**
 * Hash object.
 */
typedef struct __attribute__((__packed__)) {
    const lhash_method *mth;
    char ctx[1];
} lhash_obj;

static const lhash_method *lhash_method_lookup_by_name(const char *name) {
    for (size_t i = 0; i < HAPArrayCount(lhash_mths); i++) {
        if (HAPStringAreEqual(lhash_mths[i].name, name)) {
            return lhash_mths + i;
        }
    }
    return NULL;
}

static int lhash_new(lua_State *L, const char *name) {
    const lhash_method *mth = lhash_method_lookup_by_name(name);
    if (!mth) {
        luaL_error(L, "'%s' hash algorithm does not support", name);
    }
    lhash_obj *obj = lua_newuserdata(L, sizeof(lhash_obj) + mth->ctx_len - 1);
    luaL_setmetatable(L, LUA_HASH_OBJ_NAME);
    obj->mth = mth;
    mth->init(obj->ctx);
    return 1;
}

static int lhash_md5(lua_State *L) {
    return lhash_new(L, "md5");
}

static const luaL_Reg hashlib[] = {
    {"md5", lhash_md5},
    {NULL, NULL},
};

static int lhash_obj_update(lua_State *L) {
    size_t len;
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    if (!obj->mth) {
        luaL_error(L, "attempt to use a destroyed hash object");
    }
    const char *s = luaL_checklstring(L, 2, &len);
    lua_pushboolean(L, obj->mth->update(obj->ctx, s, len));
    return 1;
}

static int lhash_obj_digest(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    if (!obj->mth) {
        luaL_error(L, "attempt to use a destroyed hash object");
    }
    char out[obj->mth->digest_len];
    obj->mth->digest(obj->ctx, out);
    lua_pushlstring(L, out, obj->mth->digest_len);
    return 1;
}

static int lhash_obj_gc(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    obj->mth = NULL;
    return 0;
}

static int lhash_obj_tostring(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    if (obj->mth) {
        lua_pushfstring(L, "hash object (%p)", obj);
    } else {
        lua_pushliteral(L, "hash object (destroyed)");
    }
    return 1;
}

/*
 * metamethods for hash object
 */
static const luaL_Reg lhash_obj_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lhash_obj_gc},
    {"__close", lhash_obj_gc},
    {"__tostring", lhash_obj_tostring},
    {NULL, NULL}
};

/*
 * methods for hash object
 */
static const luaL_Reg lhash_obj_meth[] = {
    {"update", lhash_obj_update},
    {"digest", lhash_obj_digest},
    {NULL, NULL},
};

static void lhash_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_HASH_OBJ_NAME);  /* metatable for hash object */
    luaL_setfuncs(L, lhash_obj_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lhash_obj_meth);  /* create method table */
    luaL_setfuncs(L, lhash_obj_meth, 0);  /* add hash object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_hash(lua_State *L) {
    luaL_newlib(L, hashlib); /* new module */
    lhash_createmeta(L);
    return 1;
}
