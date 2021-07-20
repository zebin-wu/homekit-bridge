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

/**
 * Timer object.
 */
typedef struct {
    lua_State *L;
    HAPPlatformTimerRef timer;  /* Timer ID. Start from 1. */
    int argc;
    int ref_ids[1]; /* ref_ids[0] is cb ref id */
} ltimer_obj;

#define LPAL_TIMER_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_TIMER_HANDLE_NAME)

static void ltimer_obj_reset(ltimer_obj *obj) {
    lc_unref(obj->L, obj->ref_ids[0]);
    obj->ref_ids[0] = LUA_REFNIL;

    for (int i = 1; i <= obj->argc; i++) {
        lc_unref(obj->L, obj->ref_ids[i]);
        obj->ref_ids[i] = LUA_REFNIL;
    }
    obj->argc = 0;
    obj->L = NULL;
}

static void ltimer_cb(HAPPlatformTimerRef timer, void* _Nullable context) {
    ltimer_obj *obj = context;
    lua_State *L = obj->L;

    obj->timer = 0;

    if (!lc_push_ref(L, obj->ref_ids[0])) {
        HAPLogError(&ltimer_log, "%s: Can't get lua function.", __func__);
        goto end;
    }
    for (int i = 1; i <= obj->argc; i++) {
        if (!lc_push_ref(L, obj->ref_ids[i])) {
            HAPLogError(&ltimer_log, "%s: Can't get #arg %d set in timer.create().",
                __func__, i + 2);
            goto end;
        }
    }

    if (lua_pcall(L, obj->argc, 0, 0)) {
        HAPLogError(&ltimer_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
end:
    ltimer_obj_reset(obj);
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

    ltimer_obj *obj = lua_newuserdata(L, sizeof(ltimer_obj) + sizeof(int) * argc);
    luaL_setmetatable(L, LUA_TIMER_HANDLE_NAME);
    HAPError err = HAPPlatformTimerRegister(&obj->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), ltimer_cb, obj);
    if (err != kHAPError_None) {
        goto err;
    }
    obj->L = L;
    obj->argc = argc;
    obj->ref_ids[0] = lc_ref(L, 2);
    for (int i = 1; i <= argc; i++) {
        obj->ref_ids[i] = lc_ref(L, i + 2);
    }
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int ltimer_obj_cancel(lua_State *L) {
    ltimer_obj *obj = LPAL_TIMER_GET_HANDLE(L, 1);
    if (!obj->timer) {
        luaL_error(L, "attempt to use a timer that has expired");
    }
    ltimer_obj_reset(obj);
    HAPPlatformTimerDeregister(obj->timer);
    obj->timer = 0;
    return 0;
}

static int ltimer_obj_gc(lua_State *L) {
    ltimer_obj *obj = LPAL_TIMER_GET_HANDLE(L, 1);
    ltimer_obj_reset(obj);
    if (obj->timer) {
        HAPPlatformTimerDeregister(obj->timer);
        obj->timer = 0;
    }
    return 0;
}

static int ltimer_obj_tostring(lua_State *L) {
    ltimer_obj *obj = LPAL_TIMER_GET_HANDLE(L, 1);
    if (obj->timer) {
        lua_pushfstring(L, "timer (%u)", obj->timer);
    } else {
        lua_pushliteral(L, "timer (expired)");
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
static const luaL_Reg ltimer_obj_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", ltimer_obj_gc},
    {"__close", ltimer_obj_gc},
    {"__tostring", ltimer_obj_tostring},
    {NULL, NULL}
};

/*
 * methods for TimerHandle
 */
static const luaL_Reg ltimer_obj_meth[] = {
    {"cancel", ltimer_obj_cancel},
    {NULL, NULL},
};

static void ltimer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_HANDLE_NAME);  /* metatable for UDP obj */
    luaL_setfuncs(L, ltimer_obj_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ltimer_obj_meth);  /* create method table */
    luaL_setfuncs(L, ltimer_obj_meth, 0);  /* add udp obj methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_timer(lua_State *L) {
    luaL_newlib(L, ltimer_funcs);
    ltimer_createmeta(L);
    return 1;
}
