// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPPlatformTimer.h>

#include "app_int.h"
#include "lc.h"

#define LUA_TIMER_NAME "Timer*"
#define LCORE_ATEXITS "_ATEXITS"

static const HAPLogObject lcore_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ltime",
};

/**
 * Timer object context.
 */
typedef struct {
    int nargs;
    lua_State *mL;
    HAPPlatformTimerRef timer;  /* Timer ID. Start from 1. */
} lcore_timer_ctx;

static int lcore_time(lua_State *L) {
    lua_pushinteger(L, HAPPlatformClockGetCurrent() / 1000);
    return 1;
}

static int lcore_exit_finsh(lua_State *L, int status, lua_KContext extra) {
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPPlatformRunLoopStop();
        lua_error(L);
    }
    for (int i = extra; ; i++) {
        int type = lua_rawgeti(L, -1, i);
        if (type == LUA_TNIL) {
            break;
        } else if (type != LUA_TFUNCTION) {
            HAPPlatformRunLoopStop();
            luaL_error(L, "'core.onExit[%d]' must be a function", i);
        }
        lua_callk(L, 0, 0, i + 1, lcore_exit_finsh);
    }
    HAPPlatformRunLoopStop();
    return 0;
}

static int lcore_exit(lua_State *L) {
    if (luai_unlikely(!luaL_getsubtable(L, LUA_REGISTRYINDEX, LCORE_ATEXITS))) {
        HAPPlatformRunLoopStop();
        return 0;
    }
    return lcore_exit_finsh(L, 0, 1);
}

static int lcore_atexit(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    luaL_getsubtable(L, LUA_REGISTRYINDEX, LCORE_ATEXITS);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, luaL_len(L, -2) + 1);
    return 0;
}

static int lcore_sleep_resume(lua_State *L) {
    lua_State *co = lua_touserdata(L, 1);
    lua_pop(L, 1);

    int status, nres;
    status = lc_resume(co, L, 0, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    return 0;
}

static void lcore_sleep_cb(HAPPlatformTimerRef timer, void *context) {
    lua_State *co = context;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lcore_sleep_resume);
    lua_pushlightuserdata(L, co);
    int status = lua_pcall(L, 1, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int lcore_sleep(lua_State *L) {
    lua_Integer ms = luaL_checkinteger(L, 1);
    luaL_argcheck(L, ms >= 0, 1, "ms out of range");

    HAPPlatformTimerRef timer;
    if (HAPPlatformTimerRegister(&timer,
        ms ? (HAPTime)ms + HAPPlatformClockGetCurrent() : 0,
        lcore_sleep_cb, L) != kHAPError_None) {
        luaL_error(L, "failed to create a timer");
    }
    return lua_yield(L, 0);
}

static int lcore_createTimer(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    int n = lua_gettop(L);
    lcore_timer_ctx *ctx = lua_newuserdatauv(L, sizeof(lcore_timer_ctx), n);
    luaL_setmetatable(L, LUA_TIMER_NAME);
    lua_insert(L, 1);  // put the userdata at index 1
    for (int i = n; i >= 1; i--) {
        lua_setiuservalue(L, 1, i);
    }
    ctx->nargs = n - 1;
    ctx->timer = 0;
    ctx->mL = lc_getmainthread(L);
    return 1;
}

static int lcore_timer_resume(lua_State *L) {
    lcore_timer_ctx *ctx = lua_touserdata(L, -1);
    lua_pop(L, 1);

    int nres, status;
    lua_State *co = lua_newthread(L);
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, ctx) == LUA_TUSERDATA);
    if (luai_unlikely(!lua_checkstack(L, ctx->nargs + 1))) {
        luaL_error(L, "stack overflow");
    }
    for (int i = 1; i <= ctx->nargs + 1; i++) {
        lua_getiuservalue(co, 1, i);
    }
    lua_remove(co, 1);
    lua_pushnil(co);
    lua_rawsetp(co, LUA_REGISTRYINDEX, ctx);
    status = lc_resume(co, L, ctx->nargs, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    return 0;
}

static void lcore_timer_cb(HAPPlatformTimerRef timer, void *context) {
    lcore_timer_ctx *ctx = context;
    lua_State *L = ctx->mL;

    ctx->timer = 0;

    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lcore_timer_resume);
    lua_pushlightuserdata(L, ctx);
    int status = lua_pcall(L, 1, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int lcore_timer_start(lua_State *L) {
    lcore_timer_ctx *ctx = luaL_checkudata(L, 1, LUA_TIMER_NAME);

    lua_Integer ms = luaL_checkinteger(L, 2);
    luaL_argcheck(L, ms >= 0, 2, "ms out of range");

    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
    }

    if (HAPPlatformTimerRegister(&ctx->timer,
        ms ? (HAPTime)ms + HAPPlatformClockGetCurrent() : 0,
        lcore_timer_cb, ctx) != kHAPError_None) {
        luaL_error(L, "failed to start the timer");
    }
    lua_pop(L, 1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, ctx);
    return 0;
}

static int lcore_timer_stop(lua_State *L) {
    lcore_timer_ctx *ctx = luaL_checkudata(L, 1, LUA_TIMER_NAME);

    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
        ctx->timer = 0;
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, ctx);
    }
    return 0;
}

static int lcore_timer_tostring(lua_State *L) {
    lcore_timer_ctx *ctx = luaL_checkudata(L, 1, LUA_TIMER_NAME);

    if (ctx->timer) {
        lua_pushfstring(L, "timer (%u)", ctx->timer);
    } else {
        lua_pushliteral(L, "timer (expired)");
    }
    return 1;
}

static const luaL_Reg lcore_funcs[] = {
    {"time", lcore_time},
    {"exit", lcore_exit},
    {"atexit", lcore_atexit},
    {"sleep", lcore_sleep},
    {"createTimer", lcore_createTimer},
    {NULL, NULL},
};

/*
 * metamethods for timer object
 */
static const luaL_Reg lcore_timer_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lcore_timer_stop},
    {"__tostring", lcore_timer_tostring},
    {NULL, NULL}
};

/*
 * methods for timer object
 */
static const luaL_Reg lcore_timer_meth[] = {
    {"start", lcore_timer_start},
    {"stop", lcore_timer_stop},
    {NULL, NULL},
};

static void lcore_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_NAME);  /* metatable for timer object */
    luaL_setfuncs(L, lcore_timer_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_timer_meth);  /* create method table */
    luaL_setfuncs(L, lcore_timer_meth, 0);  /* add timer object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_core(lua_State *L) {
    luaL_newlib(L, lcore_funcs);
    lcore_createmeta(L);
    return 1;
}
