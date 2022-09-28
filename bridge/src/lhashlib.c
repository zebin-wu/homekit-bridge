// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/md.h>

#define LUA_HASH_OBJ_NAME "HashObject*"

#define LHASH_GET_OBJ(L, idx) \
    luaL_checkudata(L, idx, LUA_HASH_OBJ_NAME)

/**
 * Hash object.
 */
typedef struct {
    pal_md_ctx ctx;
} lhash_obj;

const char *lhash_type_strs[] = {
    "MD4",
    "MD5",
    "SHA1",
    "SHA224",
    "SHA256",
    "SHA384",
    "SHA512",
    "RIPEMD160",
    NULL
};

static int lhash_create(lua_State *L) {
    pal_md_type type = luaL_checkoption(L, 1, NULL, lhash_type_strs);
    size_t keylen = 0;
    const char *key = luaL_optlstring(L, 2, NULL, &keylen);
    lhash_obj *obj = lua_newuserdata(L, sizeof(lhash_obj));
    luaL_setmetatable(L, LUA_HASH_OBJ_NAME);
    if (luai_unlikely(!pal_md_ctx_init(&obj->ctx, type, key, keylen))) {
        luaL_error(L, "failed to create a %s context", lhash_type_strs[type]);
    }
    return 1;
}

static const luaL_Reg hashlib[] = {
    {"create", lhash_create},
    {NULL, NULL},
};

static int lhash_obj_update(lua_State *L) {
    size_t len;
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    const char *s = luaL_checklstring(L, 2, &len);
    if (luai_unlikely(!pal_md_update(&obj->ctx, s, len))) {
        luaL_error(L, "failed to update data");
    }
    lua_pushvalue(L, 1);
    return 1;
}

static int lhash_obj_digest(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    size_t len = pal_md_get_size(&obj->ctx);
    char out[len];
    if (luai_unlikely(!pal_md_digest(&obj->ctx, (uint8_t *)out))) {
        luaL_error(L, "failed to finishes the digest operation");
    }
    lua_pushlstring(L, out, len);
    return 1;
}

static int lhash_obj_hexdigest(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    size_t len = pal_md_get_size(&obj->ctx);
    char digest[len];
    if (luai_unlikely(!pal_md_digest(&obj->ctx, (uint8_t *)digest))) {
        luaL_error(L, "failed to finishes the digest operation");
    }
    size_t olen = len * 2;
    char out[olen];
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = "0123456789abcdef"[(unsigned char)digest[i] >> 4];
        out[i * 2 + 1] = "0123456789abcdef"[(unsigned char)digest[i] & 0x0F];
    }
    lua_pushlstring(L, out, olen);
    return 1;
}

static int lhash_obj_gc(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    pal_md_ctx_deinit(&obj->ctx);
    return 0;
}

static int lhash_obj_tostring(lua_State *L) {
    lhash_obj *obj = LHASH_GET_OBJ(L, 1);
    lua_pushfstring(L, "hash object (%p)", obj);
    return 1;
}

/*
 * metamethods for hash object
 */
static const luaL_Reg lhash_obj_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lhash_obj_gc},
    {"__tostring", lhash_obj_tostring},
    {NULL, NULL}
};

/*
 * methods for hash object
 */
static const luaL_Reg lhash_obj_meth[] = {
    {"update", lhash_obj_update},
    {"digest", lhash_obj_digest},
    {"hexdigest", lhash_obj_hexdigest},
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
