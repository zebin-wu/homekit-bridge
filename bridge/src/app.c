// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <lualib.h>
#include <embedfs.h>

#include <app.h>

#include "app_int.h"
#include "lc.h"

// Declare the function of lua-cjson.
#define LUA_CJSON_NAME "cjson"
extern int luaopen_cjson(lua_State *L);

#define luaL_dobufferx(L, buff, sz, name, mode) \
    (luaL_loadbufferx(L, buff, sz, name, mode) || lua_pcall(L, 0, LUA_MULTRET, 0))

// Bridge embedfs root.
extern const embedfs_dir BRIDGE_EMBEDFS_ROOT;

static lua_State *L;

static const luaL_Reg globallibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_IOLIBNAME, luaopen_io},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_LOG_NAME, luaopen_log},
    {NULL, NULL}
};

static const luaL_Reg dynamiclibs[] = {
    {LUA_HAP_NAME, luaopen_hap},
    {LUA_BOARD_NAME, luaopen_board},
    {LUA_UDP_NAME, luaopen_udp},
    {LUA_TIME_NAME, luaopen_time},
    {LUA_HASH_NAME, luaopen_hash},
    {LUA_CIPHER_NAME, luaopen_cipher},
    {LUA_CJSON_NAME, luaopen_cjson},
    {LUA_SOCKET_NAME, luaopen_socket},
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

// app_lua_run(dir: string, entry: string)
static int app_lua_run(lua_State *L) {
    const char *dir = luaL_checkstring(L, 1);

    // load global libraries
    for (const luaL_Reg *lib = globallibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // GC in generational mode
    lua_gc(L, LUA_GCGEN, 0, 0);

    // set file path
    char path[256];
    HAPAssert(HAPStringWithFormat(path, sizeof(path), "%s/?.lua;%s/?.luac", dir, dir) == kHAPError_None);
    lc_set_path(L, path);

    // set C path
    lc_set_cpath(L, "");

    // add searcher_dl to package.searcher
    lc_add_searcher(L, searcher_dl);

    // add searcher_embedfs to package.searcher
    lc_add_searcher(L, searcher_embedfs);

    // run entry
    int nres, status;
    lua_State *co = lc_newthread(L);
    lua_getglobal(co, "require");
    lua_pushvalue(L, 2);
    lua_xmove(L, co, 1);
    status = lua_resume(co, L, 1, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        lua_error(L);
        lc_freethread(co);
    }
    return 0;
}

void app_init(HAPPlatform *platform, const char *dir, const char *entry) {
    HAPPrecondition(platform);
    HAPPrecondition(dir);
    HAPPrecondition(entry);

    lhap_set_platform(platform);

    L = lua_newstate(app_lua_alloc, NULL);
    if (L == NULL) {
        HAPLogError(&kHAPLog_Default,
            "%s: Cannot create state: not enough memory", __func__);
        HAPAssertionFailure();
    }

    // call 'app_lua_init' in protected mode
    lua_pushcfunction(L, app_lua_run);
    lua_pushstring(L, dir);
    lua_pushstring(L, entry);

    // do the call
    int status = lua_pcall(L, 2, 0, 0);
    if (status) {
        const char *msg = lua_tostring(L, -1);
        HAPLogError(&kHAPLog_Default, "%s", msg);
        HAPAssertionFailure();
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

void app_deinit() {
    if (L) {
        lua_close(L);
        L = NULL;
    }

    lhap_set_platform(NULL);
}

lua_State *app_get_lua_main_thread() {
    return L;
}
