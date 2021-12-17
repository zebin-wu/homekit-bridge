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

#define LUA_TIMER_NAME "Timer*"

static const HAPLogObject ltime_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ltime",
};

/**
 * Timer object context.
 */
typedef struct {
    int nargs;
    HAPPlatformTimerRef timer;  /* Timer ID. Start from 1. */
} ltime_timer_ctx;

static void ltime_sleep_cb(HAPPlatformTimerRef timer, void *context) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = context;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&ltime_log, "%s: %s", __func__, lua_tostring(L, -1));
        lc_freethread(co);
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ltime_sleep(lua_State *L) {
    lua_Integer ms = luaL_checkinteger(L, 1);
    luaL_argcheck(L, ms >= 0, 1, "ms out of range");

    HAPPlatformTimerRef timer;
    if (HAPPlatformTimerRegister(&timer, (HAPTime)ms + HAPPlatformClockGetCurrent(),
        ltime_sleep_cb, L) != kHAPError_None) {
        luaL_error(L, "failed to create a timer");
    }
    lua_yield(L, 0);
    return 0;
}

static int ltime_createTimer(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    // pack function and args to a table
    int n = lua_gettop(L);
    lua_createtable(L, n, 1);
    luaL_setmetatable(L, LUA_TIMER_NAME);
    lua_insert(L, 1);  // put the table at index 1
    for (int i = n; i >= 1; i--) {
        lua_seti(L, 1, i);
    }
    ltime_timer_ctx *ctx = lua_newuserdata(L, sizeof(ltime_timer_ctx));
    ctx->nargs = n - 1;
    ctx->timer = 0;
    lua_setfield(L, 1, "ctx");  // t.ctx = timer object context
    return 1;  // return the table
}

static void ltime_timer_cb(HAPPlatformTimerRef timer, void *context) {
    ltime_timer_ctx *ctx = context;
    lua_State *L = app_get_lua_main_thread();

    ctx->timer = 0;

    HAPAssert(lua_gettop(L) == 0);

    int nres, status;
    lua_State *co = lua_newthread(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, co);
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, &ctx->timer) == LUA_TTABLE);
    for (int i = 1; i <= ctx->nargs + 1; i++) {
        lua_geti(co, 1, i);
    }
    lua_remove(co, 1);
    lua_pushnil(co);
    lua_rawsetp(co, LUA_REGISTRYINDEX, &ctx->timer);
    status = lua_resume(co, L, ctx->nargs, &nres);
    if (status == LUA_OK || status == LUA_YIELD) {
        if (status == LUA_OK) {
            lua_resetthread(co);
            lua_pushnil(L);
            lua_rawsetp(L, LUA_REGISTRYINDEX, co);
        }
    } else {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&ltime_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ltime_timer_start(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_Integer ms = luaL_checkinteger(L, 2);
    luaL_argcheck(L, ms >= 0, 2, "ms out of range");

    lua_getfield(L, 1, "ctx");
    ltime_timer_ctx *ctx = lua_touserdata(L, -1);
    lua_pop(L, 2);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);

    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
    }

    HAPError err = HAPPlatformTimerRegister(&ctx->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), ltime_timer_cb, ctx);
    if (err != kHAPError_None) {
        luaL_error(L, "failed to start the timer");
    }
    return 0;
}

static int ltime_timer_stop(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltime_timer_ctx *ctx = lua_touserdata(L, -1);
    if (!ctx->timer) {
        return 0;
    }
    HAPPlatformTimerDeregister(ctx->timer);
    ctx->timer = 0;
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);
    return 0;
}

static int ltime_timer_gc(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltime_timer_ctx *ctx = lua_touserdata(L, -1);
    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
        ctx->timer = 0;
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &ctx->timer);
    }
    return 0;
}

static int ltime_timer_tostring(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ctx");
    ltime_timer_ctx *ctx = lua_touserdata(L, -1);
    if (ctx->timer) {
        lua_pushfstring(L, "timer (%u)", ctx->timer);
    } else {
        lua_pushliteral(L, "timer (expired)");
    }
    return 1;
}

static const luaL_Reg ltime_funcs[] = {
    {"sleep", ltime_sleep},
    {"createTimer", ltime_createTimer},
    {NULL, NULL},
};

/*
 * metamethods for timer object
 */
static const luaL_Reg ltime_timer_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", ltime_timer_gc},
    {"__tostring", ltime_timer_tostring},
    {NULL, NULL}
};

/*
 * methods for timer object
 */
static const luaL_Reg ltime_timer_meth[] = {
    {"start", ltime_timer_start},
    {"stop", ltime_timer_stop},
    {NULL, NULL},
};

static void ltime_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_NAME);  /* metatable for timer object */
    luaL_setfuncs(L, ltime_timer_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ltime_timer_meth);  /* create method table */
    luaL_setfuncs(L, ltime_timer_meth, 0);  /* add timer object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_time(lua_State *L) {
    luaL_newlib(L, ltime_funcs);
    ltime_createmeta(L);
    return 1;
}
