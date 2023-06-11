// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <lauxlib.h>
#include <lualib.h>
#include <embedfs.h>
#include <pal/mem.h>
#include <pal/err.h>
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

struct app_exec_ctx {
    bool in_progress;
    int argc;
    const char *cmd;
    const char **argv;
    app_exec_returned_cb cb;
    void *cb_arg;
};

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
    {LUA_CJSON_NAME, luaopen_cjson},
    {NULL, NULL}
};

static const luaL_Reg dynamiclibs[] = {
    {LUA_HAP_NAME, luaopen_hap},
    {LUA_CHIP_NAME, luaopen_chip},
    {LUA_HASH_NAME, luaopen_hash},
    {LUA_CIPHER_NAME, luaopen_cipher},
    {LUA_SOCKET_NAME, luaopen_socket},
    {LUA_DNS_NAME, luaopen_dns},
    {LUA_NVS_NAME, luaopen_nvs},
    {LUA_STREAM_NAME, luaopen_stream},
    {LUA_BASE64_NAME, luaopen_base64},
    {LUA_ARC4_NAME, luaopen_arc4},
    {LUA_CLI_NAME, luaopen_cli},
    {LUA_NETIF_NAME, luaopen_netif},
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

static int readable(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

static int openjson(lua_State *L) {
    const char *filename = lua_tostring(L, lua_upvalueindex(1));
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        lua_pushfstring(L, "no file '%s'", filename);
        lua_error(L);
    }
    lua_pushvalue(L, lua_upvalueindex(2));
    char buf[512];
    luaL_Buffer B;
    luaL_buffinit(L, &B);
    while (!feof(f)) {
        size_t rc = fread(buf, 1, sizeof(buf), f);
        luaL_addlstring(&B, buf, rc);
    }
    fclose(f);
    luaL_pushresult(&B);
    int status = lua_pcall(L, 1, 1, 0);
    if (luai_unlikely(status != LUA_OK)) {
        luaL_error(L, "decode '%s' failed: %s", filename, lua_tostring(L, -1));
    }
    return 1;
}

static int searcher_json(lua_State *L) {
    size_t len;
    const char *name = luaL_checklstring(L, 1, &len);
    lua_getfield(L, lua_upvalueindex(1), "workdir");
    const char *workdir = lua_tostring(L, -1);
    if (luai_unlikely(workdir == NULL)) {
        lua_pushstring(L, "'package.workdir' must be a string");
        return 1;
    }
    lua_pushfstring(L, "%s/%s.json", workdir, name);
    const char *filename = lua_tostring(L, -1);
    if (readable(filename)) {
        lua_getfield(L, lua_upvalueindex(2), "decode");
        lua_pushcclosure(L, openjson, 2);
    } else {
        lua_pushfstring(L, "no file '%s'", filename);
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

// app_pinit(dir: lightuserdata)
static int app_pinit(lua_State *L) {
    const char *dir = lua_touserdata(L, 1);

    lua_settop(L, 0);

    // load global libraries
    for (const luaL_Reg *lib = globallibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // GC in generational mode
    lua_gc(L, LUA_GCGEN, 0, 0);

    // package.path = "${dir}/?.lua;${dir}/?.luac"
    lua_getglobal(L, "package");
    lua_pushfstring(L, "%s/?.lua;%s/?.luac", dir, dir);
    lua_setfield(L, -2, "path");

    // package.cpath = nil
    lua_pushnil(L);
    lua_setfield(L, -2, "cpath");

    // package.workdir = "${dir}"
    lua_pushstring(L, dir);
    lua_setfield(L, -2, "workdir");

    // push package.searchers to the stack
    // package.searchers = {searcher_preload, searcher_Lua, searcher_C, searcher_Croot}
    lua_getfield(L, -1, "searchers");

    // Get the length of the table 'searchers'
    int len = luaL_len(L, -1);

    // remove searchers [searcher_C, searcher_Croot] from table 'searchers'
    len -= 2;

    static const lua_CFunction searchers[] = {
        searcher_dl, searcher_embedfs, searcher_json, NULL
    };

    // add searchers to package.searchers
    for (int i = 0; searchers[i] != NULL; i++) {
        lua_pushvalue(L, -2);
        lua_getglobal(L, "cjson");
        lua_pushcclosure(L, searchers[i], 2);
        lua_rawseti(L, -2, len + i + 1);
    }
    lua_pop(L, 2);

    // set _BRIDGE_VERSION
    lua_pushstring(L, BRIDGE_VERSION);
    lua_setglobal(L, "_BRIDGE_VERSION");

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

void app_init(const char *dir) {
    HAPPrecondition(dir);

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

    // do the call
    int status = lua_pcall(L, 1, 0, 0);
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

static int finishentry(lua_State *L, int status, lua_KContext extra) {
    struct app_exec_ctx *ctx = lua_touserdata(L, 1);

    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        if (ctx->in_progress) {
            HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
            ctx->cb(PAL_ERR_UNKNOWN, ctx->cb_arg);
            return 0;
        }
        lua_error(L);
    }

    if (ctx->in_progress) {
        ctx->cb(PAL_ERR_OK, ctx->cb_arg);
    }
    return 0;
}

static int finishrequire(lua_State *L, int status, lua_KContext extra) {
    struct app_exec_ctx *ctx = lua_touserdata(L, 1);

    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        if (ctx->in_progress) {
            HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
            ctx->cb(PAL_ERR_UNKNOWN, ctx->cb_arg);
            return 0;
        }
        lua_error(L);
    }

    // stack<ctx, traceback, module>
    if (!lua_istable(L, -1) || lua_getfield(L, -1, APP_CMD_ENTRY) != LUA_TFUNCTION) {
        if (ctx->in_progress) {
            ctx->cb(PAL_ERR_OK, ctx->cb_arg);
        }
        return 0;
    }

    // stack<ctx, traceback, module, entry>
    for (int i = 0; i < ctx->argc; i++) {
        lua_pushstring(L, ctx->argv[i]);
    }
    return finishentry(L, lua_pcallk(L, ctx->argc, 0, 2, 0, finishentry), 0);
}

// app_entry(ctx: userdata, traceback: function)
static int app_entry(lua_State *L) {
    struct app_exec_ctx *ctx = lua_touserdata(L, 1);
    lua_getglobal(L, "require");
    lua_pushstring(L, ctx->cmd);
    return finishrequire(L, lua_pcallk(L, 1, 1, 2, 0, finishrequire), 0);
}

// app_pexec(ctx: lightuserdata)
static int app_pexec(lua_State *L) {
    lua_State *co = lc_newthread(L);

    lua_pushcfunction(co, app_entry);
    struct app_exec_ctx *ctx = lua_newuserdata(co, sizeof(*ctx));
    memcpy(ctx, lua_touserdata(L, 1), sizeof(*ctx));
    lc_pushtraceback(co);

    pal_err err = PAL_ERR_OK;
    app_exec_returned_cb cb = ctx->cb;
    void *cb_arg = ctx->cb_arg;

    int nres, status;
    status = lc_resume(co, L, 2, &nres);
    switch (status) {
    case LUA_OK:
        break;
    case LUA_YIELD:
        ctx->in_progress = true;
        return 0;
    default:
        HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
        err = PAL_ERR_UNKNOWN;
        break;
    }

    cb(err, cb_arg);

    return 0;
}

static void app_exec_cb(void *context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(struct app_exec_ctx));

    lua_pushcfunction(L, app_pexec);
    lua_pushlightuserdata(L, context);
    int status = lua_pcall(L, 1, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

void app_exec(const char *cmd, size_t argc, const char *argv[], app_exec_returned_cb cb, void *cb_arg) {
    HAPPrecondition(cmd);
    HAPPrecondition(argc == 0 || argv);
    HAPPrecondition(cb);

    struct app_exec_ctx ctx = {
        .in_progress = false,
        .cmd = cmd,
        .argc = argc,
        .argv = argv,
        .cb = cb,
        .cb_arg = cb_arg,
    };
    HAPAssert(HAPPlatformRunLoopScheduleCallback(app_exec_cb, &ctx, sizeof(ctx)) == kHAPError_None);
}

static int app_pexit(lua_State *L) {
    int nres, status;
    lua_State *co = lc_newthread(L);
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
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&kHAPLog_Default, "%s", lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

void app_exit() {
    HAPAssert(HAPPlatformRunLoopScheduleCallback(app_exit_cb, NULL, 0) == kHAPError_None);
}
