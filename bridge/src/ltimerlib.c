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

#define LUA_TIMER_OBJ_NAME "TimerObj*"

static const HAPLogObject ltimer_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ltimer",
};

/**
 * Timer object context.
 */
typedef struct {
    int nargs;
    lua_State *L;
    HAPPlatformTimerRef timer;  /* Timer ID. Start from 1. */
} ltimer_obj_ctx;

static int ltimer_create(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    // pack function and args to a table
    int n = lua_gettop(L);
    lua_createtable(L, n, 1);
    luaL_setmetatable(L, LUA_TIMER_OBJ_NAME);
    lua_insert(L, 1);  // put the table at index 1
    for (int i = n; i >= 1; i--) {
        lua_seti(L, 1, i);
    }
    ltimer_obj_ctx *ctx = lua_newuserdata(L, sizeof(ltimer_obj_ctx));
    ctx->L = L;
    ctx->nargs = n - 1;
    ctx->timer = 0;
    lua_setfield(L, 1, "ctx");  // t.ctx = timer object context
    return 1;  // return the table
}

static void ltimer_obj_cb(HAPPlatformTimerRef timer, void* _Nullable context) {
    ltimer_obj_ctx *ctx = context;
    lua_State *L = ctx->L;

    ctx->timer = 0;

    HAPAssert(lua_gettop(L) == 0);
    lc_push_traceback(L);
    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &ctx->timer) == LUA_TTABLE);
    for (int i = 1; i <= ctx->nargs + 1; i++) {
        lua_geti(L, 2, i);
    }
    lua_remove(L, 2);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);

    if (lua_pcall(L, ctx->nargs, 0, 1)) {
        HAPLogError(&ltimer_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ltimer_obj_start(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_Integer ms = luaL_checkinteger(L, 2);
    if (ms < 0) {
        luaL_error(L, "attemp to trigger before the current time");
    }

    lua_getfield(L, 1, "ctx");
    ltimer_obj_ctx *ctx = lua_touserdata(L, -1);
    lua_pop(L, 2);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);

    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
    }

    HAPError err = HAPPlatformTimerRegister(&ctx->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), ltimer_obj_cb, ctx);
    if (err != kHAPError_None) {
        luaL_error(L, "failed to start the timer");
    }
    return 0;
}

static int ltimer_obj_stop(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltimer_obj_ctx *ctx = lua_touserdata(L, -1);
    if (!ctx->timer) {
        return 0;
    }
    HAPPlatformTimerDeregister(ctx->timer);
    ctx->timer = 0;
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);
    return 0;
}

static int ltimer_obj_gc(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltimer_obj_ctx *ctx = lua_touserdata(L, -1);
    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
        ctx->timer = 0;
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);
    }
    return 0;
}

static int ltimer_obj_tostring(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltimer_obj_ctx *ctx = lua_touserdata(L, -1);
    if (ctx->timer) {
        lua_pushfstring(L, "timer (%u)", ctx->timer);
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
 * metamethods for timer object
 */
static const luaL_Reg ltimer_obj_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", ltimer_obj_gc},
    {"__tostring", ltimer_obj_tostring},
    {NULL, NULL}
};

/*
 * methods for timer object
 */
static const luaL_Reg ltimer_obj_meth[] = {
    {"start", ltimer_obj_start},
    {"stop", ltimer_obj_stop},
    {NULL, NULL},
};

static void ltimer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_OBJ_NAME);  /* metatable for timer object */
    luaL_setfuncs(L, ltimer_obj_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ltimer_obj_meth);  /* create method table */
    luaL_setfuncs(L, ltimer_obj_meth, 0);  /* add timer object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_timer(lua_State *L) {
    luaL_newlib(L, ltimer_funcs);
    ltimer_createmeta(L);
    return 1;
}
