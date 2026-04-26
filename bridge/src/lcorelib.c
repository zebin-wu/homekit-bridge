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
#define LUA_MQ_OBJ_NAME "MQ*"
#define LCORE_ATEXITS "_ATEXITS"

static const HAPLogObject lcore_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "core",
};

/**
 * Timer object context.
 */
typedef struct {
    int nargs;
    lua_State *mL;
    HAPPlatformTimerRef timer;  /* Timer ID. Start from 1. */
} lcore_timer_ctx;

typedef struct {
    size_t first;
    size_t last;
    size_t size;
} lcore_mq;

typedef struct {
    lua_State *co;
    HAPPlatformTimerRef timer;
    bool with_status;
} lcore_mq_wait_ctx;

static int lcore_time(lua_State *L) {
    lua_pushnumber(L, HAPPlatformClockGetCurrent());
    return 1;
}

static int lcore_exit_finish(lua_State *L, int status, lua_KContext extra) {
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
            luaL_error(L, "'registry." LCORE_ATEXITS "[%d]' must be a function", i);
        }
        int status = lua_pcallk(L, 0, 0, 1, i + 1, lcore_exit_finish);
        if (luai_unlikely(status != LUA_OK)) {
            HAPPlatformRunLoopStop();
            lua_error(L);
        }
    }
    HAPPlatformRunLoopStop();
    return 0;
}

static int lcore_exit(lua_State *L) {
    lua_settop(L, 0);
    lc_pushtraceback(L);
    if (luai_unlikely(!luaL_getsubtable(L, LUA_REGISTRYINDEX, LCORE_ATEXITS))) {
        HAPPlatformRunLoopStop();
        return 0;
    }
    return lcore_exit_finish(L, LUA_OK, 1);
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

static int lcore_create_timer(lua_State *L) {
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

static int lcore_create_mq(lua_State *L) {
    size_t size = luaL_checkinteger(L, 1);
    luaL_argcheck(L, size > 0, 1, "size out of range");
    lcore_mq *obj = lua_newuserdatauv(L, sizeof(*obj), 1);
    luaL_setmetatable(L, LUA_MQ_OBJ_NAME);
    obj->first = 1;
    obj->last = 1;
    obj->size = size;
    lua_createtable(L, 0, 1);
    lua_setuservalue(L, -2);
    return 1;
}

static int lcore_timer_resume(lua_State *L) {
    lcore_timer_ctx *ctx = lua_touserdata(L, -1);
    lua_pop(L, 1);

    int nres, status;
    lua_State *co = lc_newthread(L);
    if (luai_unlikely(lua_rawgetp(co, LUA_REGISTRYINDEX, ctx) != LUA_TUSERDATA)) {
        HAPFatalError();
    }
    if (luai_unlikely(!lua_checkstack(co, ctx->nargs + 1))) {
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

static void lcore_timer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_NAME);  /* metatable for timer object */
    luaL_setfuncs(L, lcore_timer_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_timer_meth);  /* create method table */
    luaL_setfuncs(L, lcore_timer_meth, 0);  /* add timer object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

static size_t lcore_mq_size(lcore_mq *obj) {
    return obj->first > obj->last ? obj->size - obj->first + obj->last : obj->last - obj->first;
}

static void lcore_mq_resume(lua_State *L, lua_State *co, int nargs) {
    int status, nres;
    lua_xmove(L, co, nargs);
    status = lc_resume(co, L, nargs, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_pop(L, nres);
}

static int lcore_mq_recv_ready(lua_State *L, int mq_idx, lcore_mq *obj, bool with_status) {
    mq_idx = lua_absindex(L, mq_idx);
    HAPAssert(lua_getuservalue(L, mq_idx) == LUA_TTABLE);
    int store_idx = lua_gettop(L);
    lua_geti(L, store_idx, obj->first);
    lua_pushnil(L);
    lua_seti(L, store_idx, obj->first);
    obj->first++;
    if (obj->first > obj->size + 1) {
        obj->first = 1;
    }
    int nargs = luaL_len(L, store_idx + 1);
    if (with_status) {
        lua_pushboolean(L, true);
    }
    for (int i = 1; i <= nargs; i++) {
        lua_geti(L, store_idx + 1, i);
    }
    return with_status ? nargs + 1 : nargs;
}

static bool lcore_mq_wait_remove_at(lua_State *L, int wait_idx, int pos) {
    wait_idx = lua_absindex(L, wait_idx);
    int wait_count = luaL_len(L, wait_idx);
    if (pos != wait_count) {
        lua_geti(L, wait_idx, wait_count);
        lua_seti(L, wait_idx, pos);
    }
    lua_pushnil(L);
    lua_seti(L, wait_idx, wait_count);
    return wait_count == 1;
}

static void lcore_mq_wait_remove(lua_State *L, lcore_mq_wait_ctx *ctx) {
    if (lua_rawgetp(L, LUA_REGISTRYINDEX, ctx) != LUA_TUSERDATA) {
        lua_pop(L, 1);
        return;
    }

    int ctx_idx = lua_gettop(L);
    HAPAssert(lua_getiuservalue(L, ctx_idx, 1) == LUA_TUSERDATA);
    int mq_idx = lua_gettop(L);
    HAPAssert(lua_getuservalue(L, mq_idx) == LUA_TTABLE);
    int store_idx = lua_gettop(L);
    if (lua_getfield(L, store_idx, "wait") == LUA_TTABLE) {
        int wait_idx = lua_gettop(L);
        int wait_count = luaL_len(L, wait_idx);
        for (int i = 1; i <= wait_count; i++) {
            if (lua_geti(L, wait_idx, i) == LUA_TUSERDATA && lua_touserdata(L, -1) == ctx) {
                lua_pop(L, 1);
                if (lcore_mq_wait_remove_at(L, wait_idx, i)) {
                    lua_pushnil(L);
                    lua_setfield(L, store_idx, "wait");
                }
                break;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
    }
    lua_pop(L, 3);
}

static void lcore_mq_waitctx_release(lua_State *L, int idx, bool cancel_timer) {
    idx = lua_absindex(L, idx);
    lcore_mq_wait_ctx *ctx = lua_touserdata(L, idx);

    if (cancel_timer && ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
    }
    ctx->timer = 0;

    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, ctx);
    lua_pushnil(L);
    lua_setiuservalue(L, idx, 1);
    ctx->co = NULL;
}

static int lcore_mq_wait_timeout_resume(lua_State *L) {
    lcore_mq_wait_ctx *ctx = lua_touserdata(L, 1);
    lua_pop(L, 1);

    if (lua_rawgetp(L, LUA_REGISTRYINDEX, ctx) != LUA_TUSERDATA) {
        lua_pop(L, 1);
        return 0;
    }

    lua_State *co = ctx->co;
    lcore_mq_wait_remove(L, ctx);
    lcore_mq_waitctx_release(L, -1, false);
    lua_pop(L, 1);

    if (!co) {
        return 0;
    }

    lua_pushboolean(L, false);
    lua_pushliteral(L, "timeout");
    lcore_mq_resume(L, co, 2);
    return 0;
}

static void lcore_mq_wait_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    lcore_mq_wait_ctx *ctx = context;
    if (!ctx->co) {
        return;
    }
    lua_State *L = lc_getmainthread(ctx->co);

    ctx->timer = 0;

    HAPAssert(lua_gettop(L) == 0);

    lc_pushtraceback(L);
    lua_pushcfunction(L, lcore_mq_wait_timeout_resume);
    lua_pushlightuserdata(L, ctx);
    int status = lua_pcall(L, 1, 0, 1);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int lcore_mq_wait(lua_State *L, int mq_idx, bool with_status, HAPTime deadline) {
    mq_idx = lua_absindex(L, mq_idx);
    HAPAssert(lua_getuservalue(L, mq_idx) == LUA_TTABLE);
    int store_idx = lua_gettop(L);
    int type = lua_getfield(L, store_idx, "wait");
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        lua_createtable(L, 1, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, store_idx, "wait");
    } else {
        HAPAssert(type == LUA_TTABLE);
    }

    int wait_idx = lua_gettop(L);
    lcore_mq_wait_ctx *ctx = lua_newuserdatauv(L, sizeof(*ctx), 1);
    ctx->co = L;
    ctx->timer = 0;
    ctx->with_status = with_status;
    lua_pushvalue(L, mq_idx);
    lua_setiuservalue(L, -2, 1);

    int wait_pos = luaL_len(L, wait_idx) + 1;
    lua_pushvalue(L, -1);
    lua_seti(L, wait_idx, wait_pos);
    if (deadline) {
        lua_pushvalue(L, -1);
        lua_rawsetp(L, LUA_REGISTRYINDEX, ctx);
        if (luai_unlikely(HAPPlatformTimerRegister(&ctx->timer,
            deadline, lcore_mq_wait_timeout_cb, ctx) != kHAPError_None)) {
            if (lcore_mq_wait_remove_at(L, wait_idx, wait_pos)) {
                lua_pushnil(L);
                lua_setfield(L, store_idx, "wait");
            }
            lcore_mq_waitctx_release(L, -1, false);
            luaL_error(L, "failed to create a timer");
        }
    }
    lua_pop(L, 3);
    return lua_yield(L, 0);
}

static int lcore_mq_send(lua_State *L) {
    lcore_mq *obj = luaL_checkudata(L, 1, LUA_MQ_OBJ_NAME);
    int narg = lua_gettop(L) - 1;

    lua_getuservalue(L, 1);

    if (lua_getfield(L, -1, "wait") == LUA_TTABLE) {
        lua_pushnil(L);
        lua_setfield(L, -3, "wait");  // que.wait = nil
        int waiting = luaL_len(L, -1);
        for (int i = 1; i <= waiting; i++) {
            HAPAssert(lua_geti(L, -1, i) == LUA_TUSERDATA);
            lcore_mq_wait_ctx *ctx = lua_touserdata(L, -1);
            lua_State *co = ctx->co;
            bool with_status = ctx->with_status;
            lcore_mq_waitctx_release(L, -1, true);
            lua_pop(L, 1);
            int nargs = narg + with_status;
            if (luai_unlikely(!lua_checkstack(L, nargs))) {
                luaL_error(L, "stack overflow");
            }
            if (with_status) {
                lua_pushboolean(L, true);
            }
            for (int j = 2; j <= 1 + narg; j++) {
                lua_pushvalue(L, j);
            }
            if (co) {
                lcore_mq_resume(L, co, nargs);
            }
        }
    } else {
        if (lcore_mq_size(obj) == obj->size) {
            luaL_error(L, "the message queue is full");
        }
        lua_pop(L, 1);
        lua_insert(L, 2);
        lua_createtable(L, narg, 0);
        lua_insert(L, 3);
        for (int i = narg; i >= 1; i--) {
            lua_seti(L, 3, i);
        }
        lua_seti(L, 2, obj->last);
        obj->last++;
        if (obj->last > obj->size + 1) {
            obj->last = 1;
        }
    }
    return 0;
}

static int lcore_mq_recv(lua_State *L) {
    lcore_mq *obj = luaL_checkudata(L, 1, LUA_MQ_OBJ_NAME);
    if (lua_gettop(L) != 1) {
        luaL_error(L, "invalid arguements");
    }
    if (obj->last == obj->first) {
        return lcore_mq_wait(L, 1, false, 0);
    } else {
        return lcore_mq_recv_ready(L, 1, obj, false);
    }
}

static int lcore_mq_recv_until(lua_State *L) {
    lcore_mq *obj = luaL_checkudata(L, 1, LUA_MQ_OBJ_NAME);
    lua_Integer deadline = luaL_checkinteger(L, 2);
    luaL_argcheck(L, deadline >= 0, 2, "deadline out of range");
    if (lua_gettop(L) != 2) {
        luaL_error(L, "invalid arguements");
    }
    if (obj->last != obj->first) {
        return lcore_mq_recv_ready(L, 1, obj, true);
    }

    if ((HAPTime)deadline <= HAPPlatformClockGetCurrent()) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "timeout");
        return 2;
    }
    return lcore_mq_wait(L, 1, true, (HAPTime)deadline);
}

static int lcore_mq_tostring(lua_State *L) {
    lcore_mq *obj = luaL_checkudata(L, 1, LUA_MQ_OBJ_NAME);
    lua_pushfstring(L, "message queue (%p)", obj);
    return 1;
}

/*
 * metamethods for message queue object
 */
static const luaL_Reg lcore_mq_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__tostring", lcore_mq_tostring},
    {NULL, NULL}
};

/*
 * methods for MQ object
 */
static const luaL_Reg lcore_mq_meth[] = {
    {"send", lcore_mq_send},
    {"recv", lcore_mq_recv},
    {"recvUntil", lcore_mq_recv_until},
    {NULL, NULL},
};

static void lcore_mq_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_MQ_OBJ_NAME);  /* metatable for MQ object */
    luaL_setfuncs(L, lcore_mq_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_mq_meth);  /* create method table */
    luaL_setfuncs(L, lcore_mq_meth, 0);  /* add MQ object methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

static const luaL_Reg lcore_funcs[] = {
    {"time", lcore_time},
    {"exit", lcore_exit},
    {"atexit", lcore_atexit},
    {"sleep", lcore_sleep},
    {"createTimer", lcore_create_timer},
    {"createMQ", lcore_create_mq},
    {NULL, NULL},
};

LUAMOD_API int luaopen_core(lua_State *L) {
    luaL_newlib(L, lcore_funcs);
    lcore_timer_createmeta(L);
    lcore_mq_createmeta(L);
    return 1;
}
