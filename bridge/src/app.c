// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <lualib.h>
#include <embedfs.h>
#include <pal/memory.h>
#include <app.h>

#include "app_int.h"
#include "lc.h"

// Declare the function of lua-cjson.
#define LUA_CJSON_NAME "cjson"
extern int luaopen_cjson(lua_State *L);

#ifndef BRIDGE_VERSION
#error "Please define BRIDGE_VERSION"
#endif  // BRIDGE_VERSION

#define luaL_dobufferx(L, buff, sz, name, mode) \
    (luaL_loadbufferx(L, buff, sz, name, mode) || lua_pcall(L, 0, LUA_MULTRET, 0))

// Bridge embedfs root.
extern const embedfs_dir BRIDGE_EMBEDFS_ROOT;

static lua_State *L;

static const luaL_Reg globallibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_CORE_NAME, luaopen_core},
    {LUA_LOG_NAME, luaopen_log},
    {NULL, NULL}
};

static const luaL_Reg dynamiclibs[] = {
    {LUA_HAP_NAME, luaopen_hap},
    {LUA_CHIP_NAME, luaopen_chip},
    {LUA_HASH_NAME, luaopen_hash},
    {LUA_CIPHER_NAME, luaopen_cipher},
    {LUA_CJSON_NAME, luaopen_cjson},
    {LUA_SOCKET_NAME, luaopen_socket},
    {LUA_SSL_NAME, luaopen_ssl},
    {LUA_DNS_NAME, luaopen_dns},
    {LUA_NVS_NAME, luaopen_nvs},
    {LUA_STREAM_NAME, luaopen_stream},
    {LUA_BASE64_NAME, luaopen_base64},
    {LUA_ARC4_NAME, luaopen_arc4},
    {NULL, NULL}
};

static int searcher_dl(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const luaL_Reg *lib = dynamiclibs;

    for (; lib->func; lib++) {
        if (HAPStringAreEqual(lib->name, name)) {
            break;
        }
    }
    if (lib->func) {
        lua_pushcfunction(L, lib->func);
    } else {
        lua_pushfstring(L, "no module '%s' in dynamiclibs", name);
    }
    return 1;
}

static void gen_filename(const char *name, char *buf) {
    for (; *name; name++) {
        if (*name == '.') {
            *buf++ = '/';
        } else {
            *buf++ = *name;
        }
    }
    HAPRawBufferCopyBytes(buf, ".luac", sizeof(".luac"));
}

static int searcher_embedfs(lua_State *L) {
    size_t len;
    const char *name = luaL_checklstring(L, 1, &len);
    char filename[len + sizeof(".luac")];

    gen_filename(name, filename);
    const embedfs_file *file = embedfs_find_file(&BRIDGE_EMBEDFS_ROOT, filename);
    if (file) {
        luaL_loadbufferx(L, file->data, file->len, NULL, "const");
    } else {
        lua_pushfstring(L, "no file '%s' in bridge embedfs", filename);
    }
    return 1;
}

static void *app_lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud; (void)osize; /* not used */
    if (nsize == 0) {
        pal_mem_free(ptr);
        return NULL;
    } else {
        return pal_mem_realloc(ptr, nsize);
    }
}

static int finshexit(lua_State *L, int status, lua_KContext extra) {
    return 0;
}

static int finshentry(lua_State *L, int status, lua_KContext extra) {
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
        lua_getglobal(L, "core");
        lua_getfield(L, -1, "exit");
        lua_callk(L, 0, 0, 0, finshexit);
    }
    return 0;
}

// app_entry(traceback: function, entry: string)
static int app_entry(lua_State *L) {
    lua_getglobal(L, "require");
    lua_insert(L, 2);
    return finshentry(L, lua_pcallk(L, 1, 0, 1, 0, finshentry), 0);
}

// app_pinit(dir: lightuserdata, entry: lightuserdata)
static int app_pinit(lua_State *L) {
    const char *dir = lua_touserdata(L, 1);
    const char *entry = lua_touserdata(L, 2);

    lua_settop(L, 0);

    // load global libraries
    for (const luaL_Reg *lib = globallibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // GC in generational mode
    lua_gc(L, LUA_GCGEN, 0, 0);

    // set file path
    lua_getglobal(L, "package");
    lua_pushfstring(L, "%s/?.lua;%s/?.luac", dir, dir);
    lua_setfield(L, -2, "path");

    // set C path
    lua_pushstring(L, "");
    lua_setfield(L, -2, "cpath");
    lua_pop(L, 1);

    // push package.searchers to the stack
    // package.searchers = {searcher_preload, searcher_Lua, searcher_C, searcher_Croot}
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    lua_remove(L, -2);

    // Get the length of the table 'searchers'
    int len = luaL_len(L, -1);

    // remove searchers [searcher_C, searcher_Croot] from table 'searchers'
    len -= 2;

    static const lua_CFunction searchers[] = {
        searcher_dl, searcher_embedfs, NULL
    };

    // add searchers to package.searchers
    for (int i = 0; searchers[i] != NULL; i++) {
        lua_pushcfunction(L, searchers[i]);
        lua_rawseti(L, -2, len + i + 1);
    }

    // set _BRIDGE_VERSION
    lua_pushstring(L, BRIDGE_VERSION);
    lua_setglobal(L, "_BRIDGE_VERSION");

    // run entry
    int nres, status;
    lua_State *co = lua_newthread(L);
    lua_pushcfunction(co, app_entry);
    lc_pushtraceback(co);
    lua_pushstring(co, entry);
    status = lc_resume(co, L, 2, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        lua_error(L);
    }
    return 0;
}

static int panic(lua_State *L) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) {
        msg = "error object is not a string";
    }
    HAPLogError(&kHAPLog_Default, "PANIC: unprotected error in call to Lua API (%s)\n", msg);
    return 0;  /* return to Lua to abort */
}

void app_init(const char *dir, const char *entry) {
    HAPPrecondition(dir);
    HAPPrecondition(entry);

    L = lua_newstate(app_lua_alloc, NULL);
    if (luai_unlikely(!L)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Cannot create state: not enough memory", __func__);
        HAPFatalError();
    }

    lua_atpanic(L, &panic);

    // call 'app_pinit' in protected mode
    lua_pushcfunction(L, app_pinit);
    lua_pushlightuserdata(L, (void *)dir);
    lua_pushlightuserdata(L, (void *)entry);

    // do the call
    int status = lua_pcall(L, 2, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        const char *msg = lua_tostring(L, -1);
        HAPLogError(&kHAPLog_Default, "%s", msg);
        HAPFatalError();
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

void app_deinit() {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

static int app_pexit(lua_State *L) {
    int nres, status;
    lua_State *co = lua_newthread(L);
    lua_getglobal(co, "core");
    lua_getfield(co, -1, "exit");
    lua_remove(co, -2);
    status = lc_resume(co, L, 0, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        lua_error(L);
    }
    return 0;
}

static void app_exit_cb(void *context, size_t contextSize) {
    lua_pushcfunction(L, app_pexit);
    int status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK && status != LUA_YIELD) {
        HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
    }
}

void app_exit() {
    HAPAssert(HAPPlatformRunLoopScheduleCallback(app_exit_cb, NULL, 0) == kHAPError_None);
}
