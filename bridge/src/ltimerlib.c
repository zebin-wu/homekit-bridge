// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPPlatformTimer.h>

#include "app_int.h"
#include "lc.h"

#include "ltimerlib.h"

#define LUA_TIMER_HANDLE_NAME "TimerHandle"

static const HAPLogObject ltimer_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ltimer",
};

typedef struct {
    lua_State *L;
    HAPPlatformTimerRef timer;
    int argc;
    int ref_ids[1]; /* ref_ids[0] is cb ref id */
} ltimer_handle;

#define LPAL_TIMER_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_TIMER_HANDLE_NAME)

static void ltimer_handle_reset(ltimer_handle *handle) {
    lc_unref(handle->L, handle->ref_ids[0]);
    handle->ref_ids[0] = LUA_REFNIL;

    for (int i = 1; i <= handle->argc; i++) {
        lc_unref(handle->L, handle->ref_ids[i]);
        handle->ref_ids[i] = LUA_REFNIL;
    }
    handle->argc = 0;
    handle->L = NULL;
}

static void ltimer_cb(HAPPlatformTimerRef timer, void* _Nullable context) {
    ltimer_handle *handle = context;
    lua_State *L = handle->L;

    handle->timer = 0;

    if (!lc_push_ref(L, handle->ref_ids[0])) {
        HAPLogError(&ltimer_log, "%s: Can't get lua function.", __func__);
        goto end;
    }
    for (int i = 1; i <= handle->argc; i++) {
        if (!lc_push_ref(L, handle->ref_ids[i])) {
            HAPLogError(&ltimer_log, "%s: Can't get #arg %d set in timer.create().",
                __func__, i + 2);
            goto end;
        }
    }

    if (lua_pcall(L, handle->argc, 0, 0)) {
        HAPLogError(&ltimer_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
end:
    ltimer_handle_reset(handle);
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ltimer_create(lua_State *L) {
    int argc = lua_gettop(L) - 2;
    lua_Integer ms = luaL_checkinteger(L, 1);
    if (ms < 0) {
        goto err;
    }
    luaL_checktype(L, 2, LUA_TFUNCTION);

    ltimer_handle *handle = lua_newuserdata(L, sizeof(ltimer_handle) + sizeof(int) * argc);
    luaL_setmetatable(L, LUA_TIMER_HANDLE_NAME);
    HAPError err = HAPPlatformTimerRegister(&handle->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), ltimer_cb, handle);
    if (err != kHAPError_None) {
        goto err;
    }
    handle->L = L;
    handle->argc = argc;
    handle->ref_ids[0] = lc_ref(L, 2);
    for (int i = 1; i <= argc; i++) {
        handle->ref_ids[i] = lc_ref(L, i + 2);
    }
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int ltimer_handle_destroy(lua_State *L) {
    ltimer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    if (!handle->timer) {
        luaL_error(L, "attempt to use a destroyed timer");
    }
    ltimer_handle_reset(handle);
    HAPPlatformTimerDeregister(handle->timer);
    handle->timer = 0;
    return 0;
}

static int ltimer_handle_gc(lua_State *L) {
    ltimer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    ltimer_handle_reset(handle);
    if (handle->timer) {
        HAPPlatformTimerDeregister(handle->timer);
        handle->timer = 0;
    }
    return 0;
}

static int ltimer_handle_tostring(lua_State *L) {
    ltimer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    if (handle->timer) {
        lua_pushfstring(L, "timer (%p)", handle->timer);
    } else {
        lua_pushliteral(L, "timer (destroyed)");
    }
    return 1;
}

static const luaL_Reg ltimer_funcs[] = {
    {"create", ltimer_create},
    {NULL, NULL},
};

/*
 * metamethods for TimerHandle
 */
static const luaL_Reg ltimer_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", ltimer_handle_gc},
    {"__close", ltimer_handle_gc},
    {"__tostring", ltimer_handle_tostring},
    {NULL, NULL}
};

/*
 * methods for UdpHandle
 */
static const luaL_Reg ltimer_handle_meth[] = {
    {"destroy", ltimer_handle_destroy},
    {NULL, NULL},
};

static void ltimer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_HANDLE_NAME);  /* metatable for UDP handle */
    luaL_setfuncs(L, ltimer_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ltimer_handle_meth);  /* create method table */
    luaL_setfuncs(L, ltimer_handle_meth, 0);  /* add udp handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_timer(lua_State *L) {
    luaL_newlib(L, ltimer_funcs);
    ltimer_createmeta(L);
    return 1;
}
