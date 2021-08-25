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
    struct {
        int cb;
        int arg;
    } ref_ids;
} ltimer_obj;

#define LPAL_TIMER_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_TIMER_HANDLE_NAME)

static void ltimer_obj_reset(ltimer_obj *obj) {
    lc_unref(obj->L, obj->ref_ids.cb);
    obj->ref_ids.cb = LUA_REFNIL;
    lc_unref(obj->L, obj->ref_ids.arg);
    obj->ref_ids.arg = LUA_REFNIL;
}

static int ltimer_create(lua_State *L) {
    ltimer_obj *obj = lua_newuserdata(L, sizeof(ltimer_obj));
    luaL_setmetatable(L, LUA_TIMER_HANDLE_NAME);
    obj->L = L;
    return 1;
}

static void ltimer_obj_cb(HAPPlatformTimerRef timer, void* _Nullable context) {
    ltimer_obj *obj = context;
    lua_State *L = obj->L;

    obj->timer = 0;

    lc_push_traceback(L);
    HAPAssert(lc_push_ref(L, obj->ref_ids.cb));

    bool has_arg = lc_push_ref(L, obj->ref_ids.arg);
    ltimer_obj_reset(obj);
    if (lua_pcall(L, has_arg ? 1 : 0, 0, 1)) {
        HAPLogError(&ltimer_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ltimer_obj_start(lua_State *L) {
    ltimer_obj *obj = LPAL_TIMER_GET_HANDLE(L, 1);
    lua_Integer ms = luaL_checkinteger(L, 2);
    if (ms <= 0) {
        luaL_error(L, "attemp to trigger before the current time");
    }
    luaL_checktype(L, 3, LUA_TFUNCTION);

    lc_unref(L, obj->ref_ids.cb);
    obj->ref_ids.cb = lc_ref(L, 3);

    lc_unref(L, obj->ref_ids.arg);
    if (lua_isnil(L, 4)) {
        obj->ref_ids.arg = LUA_REFNIL;
    } else {
        obj->ref_ids.arg = lc_ref(L, 4);
    }

    HAPError err = HAPPlatformTimerRegister(&obj->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), ltimer_obj_cb, obj);
    if (err != kHAPError_None) {
        luaL_error(L, "failed to start the timer");
    }
    return 0;
}

static int ltimer_obj_cancel(lua_State *L) {
    ltimer_obj *obj = LPAL_TIMER_GET_HANDLE(L, 1);
    ltimer_obj_reset(obj);
    if (obj->timer) {
        HAPPlatformTimerDeregister(obj->timer);
        obj->timer = 0;
    }
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
    {"__tostring", ltimer_obj_tostring},
    {NULL, NULL}
};

/*
 * methods for TimerHandle
 */
static const luaL_Reg ltimer_obj_meth[] = {
    {"start", ltimer_obj_start},
    {"cancel", ltimer_obj_cancel},
    {NULL, NULL},
};

static void ltimer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_HANDLE_NAME);  /* metatable for timer handle */
    luaL_setfuncs(L, ltimer_obj_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ltimer_obj_meth);  /* create method table */
    luaL_setfuncs(L, ltimer_obj_meth, 0);  /* add timer handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_timer(lua_State *L) {
    luaL_newlib(L, ltimer_funcs);
    ltimer_createmeta(L);
    return 1;
}
