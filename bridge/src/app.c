// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <HAP.h>
#include <HAPAccessorySetup.h>
#include <lauxlib.h>
#include <lualib.h>

#include <app.h>

#include "app_int.h"
#include "lc.h"
#include "lhaplib.h"
#include "lboardlib.h"
#include "lnetlib.h"
#include "lloglib.h"
#include "ltimerlib.h"
#include "lhashlib.h"
#include "lcipherlib.h"

// Declare the function of lua-cjson.
#define LUA_CJSON_NAME "cjson"
extern int luaopen_cjson(lua_State *L);

/**
 * Global accessory configuration.
 */
typedef struct {
    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef kv_store;
} app_accessory_conf;

static app_context gv_app_context;
static app_accessory_conf gv_acc_conf;

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
    {LUA_NET_UDP_NAME, luaopen_net_udp},
    {LUA_TIMER_NAME, luaopen_timer},
    {LUA_HASH_NAME, luaopen_hash},
    {LUA_CIPHER_NAME, luaopen_cipher},
    {LUA_CJSON_NAME, luaopen_cjson},
    {NULL, NULL}
};

static inline const luaL_Reg *find_dl(const char *name) {
    for (const luaL_Reg *lib = dynamiclibs; lib->func; lib++) {
        if (HAPStringAreEqual(lib->name, name)) {
            return lib;
        }
    }
    return NULL;
}

static int searcher_dl(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    const luaL_Reg *lib = find_dl(name);
    if (lib) {
        lua_pushcfunction(L, lib->func);
    } else {
        lua_pushfstring(L, "no module '%s' in dynamiclibs", name);
    }
    return 1;
}

size_t app_lua_run(const char *dir, const char *entry) {
    HAPPrecondition(dir);
    HAPPrecondition(entry);

    char path[256];

    lua_State *L  = luaL_newstate();
    if (L == NULL) {
        HAPLogError(&kHAPLog_Default, "Cannot create state: not enough memory");
        goto err;
    }

    // load global libraries
    for (const luaL_Reg *lib = globallibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // set search path
    HAPAssert(HAPStringWithFormat(path, sizeof(path), "%s/?.lua;%s/?.luac", dir, dir) == kHAPError_None);
    lc_set_path(L, path);

    // add searcher_dl to package.searcher
    lc_add_searcher(L, searcher_dl);

    // run entry scripts
    HAPAssert(HAPStringWithFormat(path, sizeof(path), "%s/%s.luac", dir, entry) == kHAPError_None);
    int status = luaL_dofile(L, path);
    lc_collectgarbage(L);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        HAPLogError(&kHAPLog_Default, "%s.luac: %s", entry, msg);
        goto err1;
    }

    if (!lua_isboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default,
            "%s.luac returned is not a boolean.", entry);
        goto err1;
    }

    if (!lua_toboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default, "Failed to configure.");
        goto err1;
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
    gv_app_context.L = L;
    return lhap_get_attribute_count();
err1:
    lua_settop(L, 0);
    lc_collectgarbage(L);
err:
    HAPAssertionFailure();
}

void app_lua_close(void) {
    lua_State *L = gv_app_context.L;
    if (L) {
        lhap_unconfigure(L);
        lua_close(L);
        gv_app_context.L = NULL;
    }
}

void app_create(HAPAccessoryServerRef *server, HAPPlatformKeyValueStoreRef kv_store) {
    HAPPrecondition(server);
    HAPPrecondition(kv_store);

    HAPRawBufferZero(&gv_acc_conf, sizeof(gv_acc_conf));
    gv_acc_conf.server = server;
    gv_acc_conf.kv_store = kv_store;
    lhap_set_server(server);
}

void app_release(void) {
    lhap_set_server(NULL);
}

void app_accessory_server_start(void) {
    const lhap_conf conf = lhap_get_conf();

    if (conf.bridge_accs) {
        HAPAccessoryServerStartBridge(gv_acc_conf.server, conf.primary_acc,
            conf.bridge_accs, conf.conf_changed);
    } else {
        HAPAccessoryServerStart(gv_acc_conf.server, conf.primary_acc);
    }
}

void app_accessory_server_handle_update_state(HAPAccessoryServerRef *server, void *_Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(gv_app_context.L);
    lhap_server_handle_update_state(gv_app_context.L, HAPAccessoryServerGetState(server));
}

void app_accessory_server_handle_session_accept(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(gv_app_context.L);
    lhap_server_handle_session_accept(gv_app_context.L);
}

void app_accessory_server_handle_session_invalidate(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(gv_app_context.L);
    lhap_server_handle_session_invalidate(gv_app_context.L);
}

void app_init(
        HAPAccessoryServerOptions *server_opts,
        HAPPlatform *platform,
        HAPAccessoryServerCallbacks *server_cbs,
        void *_Nonnull *_Nonnull pcontext) {
    HAPPrecondition(pcontext);
    *pcontext = &gv_app_context;

    // Display setup code.
    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(platform->accessorySetup, &setupCode);
    HAPLogInfo(&kHAPLog_Default, "Setup code: %s", setupCode.stringValue);
}

void app_deinit() {
    /*no-op*/
}
