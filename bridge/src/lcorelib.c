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
#define LUA_FUTURE_OBJ_NAME "Future*"
#define LUA_EVENT_OBJ_NAME "Event*"
#define LUA_QUEUE_OBJ_NAME "Queue*"
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

typedef enum {
    LCORE_FUTURE_PENDING,
    LCORE_FUTURE_RESOLVED,
    LCORE_FUTURE_REJECTED,
} lcore_future_state;

typedef struct {
    lcore_future_state state;
} lcore_future;

typedef struct {
    bool set;
} lcore_event;

typedef struct {
    size_t first;
    size_t last;
    size_t size;
} lcore_queue;

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

static int lcore_isyieldable(lua_State *L) {
    lua_pushboolean(L, lua_isyieldable(L));
    return 1;
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

static char lcore_nil_sentinel;

static int lcore_checkyieldable(lua_State *L) {
    if (luai_unlikely(!lua_isyieldable(L))) {
        return luaL_error(L, "current context is not yieldable");
    }
    return 0;
}

static void lcore_pack_values(lua_State *L, int start, int narg) {
    lua_createtable(L, narg, 1);
    for (int i = 0; i < narg; i++) {
        if (lua_type(L, start + i) == LUA_TNIL) {
            lua_pushlightuserdata(L, (void *)&lcore_nil_sentinel);
        } else {
            lua_pushvalue(L, start + i);
        }
        lua_seti(L, -2, i + 1);
    }
    lua_pushinteger(L, narg);
    lua_setfield(L, -2, "n");
}

static int lcore_unpack_values(lua_State *L, int idx) {
    idx = lua_absindex(L, idx);
    if (luai_unlikely(lua_getfield(L, idx, "n") != LUA_TNUMBER)) {
        luaL_error(L, "corrupted core object state");
    }
    int narg = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(!lua_checkstack(L, narg))) {
        luaL_error(L, "stack overflow");
    }
    for (int i = 1; i <= narg; i++) {
        lua_geti(L, idx, i);
        if (lua_islightuserdata(L, -1) &&
                lua_touserdata(L, -1) == (void *)&lcore_nil_sentinel) {
            lua_pop(L, 1);
            lua_pushnil(L);
        }
    }
    return narg;
}

static void lcore_waiters_add(lua_State *L, int idx, const char *field) {
    idx = lua_absindex(L, idx);
    int type = lua_getfield(L, idx, field);
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        lua_createtable(L, 1, 0);
        lua_pushthread(L);
        lua_seti(L, -2, 1);
        lua_setfield(L, idx, field);
    } else {
        HAPAssert(type == LUA_TTABLE);
        lua_pushthread(L);
        lua_seti(L, -2, luaL_len(L, -2) + 1);
        lua_pop(L, 1);
    }
}

static lua_State *lcore_waiters_take_first(lua_State *L, int idx, const char *field) {
    idx = lua_absindex(L, idx);
    int type = lua_getfield(L, idx, field);
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        return NULL;
    }

    HAPAssert(type == LUA_TTABLE);
    int waiting = luaL_len(L, -1);
    if (waiting == 0) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_setfield(L, idx, field);
        return NULL;
    }

    HAPAssert(lua_geti(L, -1, 1) == LUA_TTHREAD);
    lua_State *co = lua_tothread(L, -1);
    lua_pop(L, 1);
    for (int i = 2; i <= waiting; i++) {
        lua_geti(L, -1, i);
        lua_seti(L, -2, i - 1);
    }
    lua_pushnil(L);
    lua_seti(L, -2, waiting);

    if (waiting == 1) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_setfield(L, idx, field);
    } else {
        lua_pop(L, 1);
    }
    return co;
}

static void lcore_resume_thread(lua_State *L, lua_State *co, int arg_start, int narg) {
    int status, nres;

    if (luai_unlikely(!lua_checkstack(co, narg))) {
        luaL_error(L, "stack overflow");
    }
    for (int i = 0; i < narg; i++) {
        lua_pushvalue(L, arg_start + i);
    }
    lua_xmove(L, co, narg);
    status = lc_resume(co, L, narg, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lcore_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_pop(L, nres);
}

static void lcore_waiters_broadcast(lua_State *L, int idx, const char *field, int arg_start, int narg) {
    idx = lua_absindex(L, idx);
    int type = lua_getfield(L, idx, field);
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        return;
    }

    HAPAssert(type == LUA_TTABLE);
    lua_pushnil(L);
    lua_setfield(L, idx, field);

    int waiting = luaL_len(L, -1);
    for (int i = 1; i <= waiting; i++) {
        HAPAssert(lua_geti(L, -1, i) == LUA_TTHREAD);
        lua_State *co = lua_tothread(L, -1);
        lua_pop(L, 1);
        lcore_resume_thread(L, co, arg_start, narg);
    }
    lua_pop(L, 1);
}

static const char *lcore_future_state_strs[] = {
    "pending",
    "resolved",
    "rejected",
    NULL,
};

static int lcore_create_future(lua_State *L) {
    lcore_future *obj = lua_newuserdatauv(L, sizeof(*obj), 1);
    luaL_setmetatable(L, LUA_FUTURE_OBJ_NAME);
    obj->state = LCORE_FUTURE_PENDING;
    lua_createtable(L, 0, 2);
    lua_setuservalue(L, -2);
    return 1;
}

static int lcore_future_complete(lua_State *L, lcore_future_state state) {
    lcore_future *obj = luaL_checkudata(L, 1, LUA_FUTURE_OBJ_NAME);
    if (luai_unlikely(obj->state != LCORE_FUTURE_PENDING)) {
        return luaL_error(L, "future is already completed");
    }

    int narg = lua_gettop(L) - 1;
    lua_getuservalue(L, 1);
    int uv = lua_gettop(L);
    lcore_pack_values(L, 2, narg);
    lua_setfield(L, uv, "result");
    obj->state = state;

    lua_pushboolean(L, state == LCORE_FUTURE_RESOLVED);
    lua_insert(L, 2);
    lcore_waiters_broadcast(L, uv + 1, "wait", 2, narg + 1);
    lua_pop(L, 1);
    return 0;
}

static int lcore_future_resolve(lua_State *L) {
    return lcore_future_complete(L, LCORE_FUTURE_RESOLVED);
}

static int lcore_future_reject(lua_State *L) {
    return lcore_future_complete(L, LCORE_FUTURE_REJECTED);
}

static int lcore_future_wait(lua_State *L) {
    lcore_future *obj = luaL_checkudata(L, 1, LUA_FUTURE_OBJ_NAME);
    if (obj->state == LCORE_FUTURE_PENDING) {
        lcore_checkyieldable(L);
        lua_getuservalue(L, 1);
        lcore_waiters_add(L, -1, "wait");
        lua_pop(L, 1);
        return lua_yield(L, 0);
    }

    lua_getuservalue(L, 1);
    lua_pushboolean(L, obj->state == LCORE_FUTURE_RESOLVED);
    if (luai_unlikely(lua_getfield(L, 2, "result") != LUA_TTABLE)) {
        luaL_error(L, "corrupted future state");
    }
    int narg = lcore_unpack_values(L, -1);
    lua_remove(L, lua_gettop(L) - narg);
    lua_remove(L, 2);
    return narg + 1;
}

static int lcore_future_getstate(lua_State *L) {
    lcore_future *obj = luaL_checkudata(L, 1, LUA_FUTURE_OBJ_NAME);
    lua_pushstring(L, lcore_future_state_strs[obj->state]);
    return 1;
}

static int lcore_future_tostring(lua_State *L) {
    lcore_future *obj = luaL_checkudata(L, 1, LUA_FUTURE_OBJ_NAME);
    lua_pushfstring(L, "future (%p, %s)", obj, lcore_future_state_strs[obj->state]);
    return 1;
}

/*
 * metamethods for future object
 */
static const luaL_Reg lcore_future_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__tostring", lcore_future_tostring},
    {NULL, NULL}
};

/*
 * methods for future object
 */
static const luaL_Reg lcore_future_meth[] = {
    {"resolve", lcore_future_resolve},
    {"reject", lcore_future_reject},
    {"wait", lcore_future_wait},
    {"state", lcore_future_getstate},
    {NULL, NULL},
};

static void lcore_future_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_FUTURE_OBJ_NAME);  /* metatable for future object */
    luaL_setfuncs(L, lcore_future_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_future_meth);  /* create method table */
    luaL_setfuncs(L, lcore_future_meth, 0);  /* add future methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

static int lcore_create_event(lua_State *L) {
    lcore_event *obj = lua_newuserdatauv(L, sizeof(*obj), 1);
    luaL_setmetatable(L, LUA_EVENT_OBJ_NAME);
    obj->set = false;
    lua_createtable(L, 0, 1);
    lua_setuservalue(L, -2);
    return 1;
}

static int lcore_event_wait(lua_State *L) {
    lcore_event *obj = luaL_checkudata(L, 1, LUA_EVENT_OBJ_NAME);
    if (obj->set) {
        return 0;
    }

    lcore_checkyieldable(L);
    lua_getuservalue(L, 1);
    lcore_waiters_add(L, -1, "wait");
    lua_pop(L, 1);
    return lua_yield(L, 0);
}

static int lcore_event_set(lua_State *L) {
    lcore_event *obj = luaL_checkudata(L, 1, LUA_EVENT_OBJ_NAME);
    obj->set = true;
    lua_getuservalue(L, 1);
    lcore_waiters_broadcast(L, -1, "wait", 0, 0);
    lua_pop(L, 1);
    return 0;
}

static int lcore_event_clear(lua_State *L) {
    lcore_event *obj = luaL_checkudata(L, 1, LUA_EVENT_OBJ_NAME);
    obj->set = false;
    return 0;
}

static int lcore_event_isset(lua_State *L) {
    lcore_event *obj = luaL_checkudata(L, 1, LUA_EVENT_OBJ_NAME);
    lua_pushboolean(L, obj->set);
    return 1;
}

static int lcore_event_tostring(lua_State *L) {
    lcore_event *obj = luaL_checkudata(L, 1, LUA_EVENT_OBJ_NAME);
    lua_pushfstring(L, "event (%p, %s)", obj, obj->set ? "set" : "clear");
    return 1;
}

/*
 * metamethods for event object
 */
static const luaL_Reg lcore_event_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__tostring", lcore_event_tostring},
    {NULL, NULL}
};

/*
 * methods for event object
 */
static const luaL_Reg lcore_event_meth[] = {
    {"wait", lcore_event_wait},
    {"set", lcore_event_set},
    {"clear", lcore_event_clear},
    {"isSet", lcore_event_isset},
    {NULL, NULL},
};

static void lcore_event_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_EVENT_OBJ_NAME);  /* metatable for event object */
    luaL_setfuncs(L, lcore_event_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_event_meth);  /* create method table */
    luaL_setfuncs(L, lcore_event_meth, 0);  /* add event methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

static size_t lcore_queue_size(lcore_queue *obj) {
    return obj->first > obj->last ? obj->size + 1 - obj->first + obj->last : obj->last - obj->first;
}

static int lcore_create_queue(lua_State *L) {
    size_t size = luaL_checkinteger(L, 1);
    luaL_argcheck(L, size > 0, 1, "size out of range");

    lcore_queue *obj = lua_newuserdatauv(L, sizeof(*obj), 1);
    luaL_setmetatable(L, LUA_QUEUE_OBJ_NAME);
    obj->first = 1;
    obj->last = 1;
    obj->size = size;
    lua_createtable(L, 0, 1);
    lua_setuservalue(L, -2);
    return 1;
}

static int lcore_queue_send(lua_State *L) {
    lcore_queue *obj = luaL_checkudata(L, 1, LUA_QUEUE_OBJ_NAME);
    int narg = lua_gettop(L) - 1;

    lua_getuservalue(L, 1);
    lua_State *co = lcore_waiters_take_first(L, -1, "recv_wait");
    if (co != NULL) {
        lua_pop(L, 1);
        lcore_resume_thread(L, co, 2, narg);
        return 0;
    }

    if (lcore_queue_size(obj) == obj->size) {
        return luaL_error(L, "queue is full");
    }

    int uv = lua_gettop(L);
    lcore_pack_values(L, 2, narg);
    lua_seti(L, uv, obj->last);
    obj->last++;
    if (obj->last > obj->size + 1) {
        obj->last = 1;
    }
    lua_pop(L, 1);
    return 0;
}

static int lcore_queue_recv(lua_State *L) {
    lcore_queue *obj = luaL_checkudata(L, 1, LUA_QUEUE_OBJ_NAME);
    lua_getuservalue(L, 1);
    int uv = lua_gettop(L);

    if (obj->last == obj->first) {
        lcore_checkyieldable(L);
        lcore_waiters_add(L, uv, "recv_wait");
        lua_pop(L, 1);
        return lua_yield(L, 0);
    }

    if (luai_unlikely(lua_geti(L, uv, obj->first) != LUA_TTABLE)) {
        luaL_error(L, "corrupted queue state");
    }
    lua_pushnil(L);
    lua_seti(L, uv, obj->first);
    obj->first++;
    if (obj->first > obj->size + 1) {
        obj->first = 1;
    }
    int narg = lcore_unpack_values(L, -1);
    lua_remove(L, lua_gettop(L) - narg);
    lua_remove(L, uv);
    return narg;
}

static int lcore_queue_len(lua_State *L) {
    lcore_queue *obj = luaL_checkudata(L, 1, LUA_QUEUE_OBJ_NAME);
    lua_pushinteger(L, lcore_queue_size(obj));
    return 1;
}

static int lcore_queue_cap(lua_State *L) {
    lcore_queue *obj = luaL_checkudata(L, 1, LUA_QUEUE_OBJ_NAME);
    lua_pushinteger(L, obj->size);
    return 1;
}

static int lcore_queue_tostring(lua_State *L) {
    lcore_queue *obj = luaL_checkudata(L, 1, LUA_QUEUE_OBJ_NAME);
    lua_pushfstring(L, "queue (%p, %d/%d)", obj, (int)lcore_queue_size(obj), (int)obj->size);
    return 1;
}

/*
 * metamethods for queue object
 */
static const luaL_Reg lcore_queue_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__tostring", lcore_queue_tostring},
    {NULL, NULL}
};

/*
 * methods for queue object
 */
static const luaL_Reg lcore_queue_meth[] = {
    {"send", lcore_queue_send},
    {"recv", lcore_queue_recv},
    {"len", lcore_queue_len},
    {"cap", lcore_queue_cap},
    {NULL, NULL},
};

static void lcore_queue_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_QUEUE_OBJ_NAME);  /* metatable for queue object */
    luaL_setfuncs(L, lcore_queue_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcore_queue_meth);  /* create method table */
    luaL_setfuncs(L, lcore_queue_meth, 0);  /* add queue methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

static const luaL_Reg lcore_funcs[] = {
    {"time", lcore_time},
    {"exit", lcore_exit},
    {"atexit", lcore_atexit},
    {"isYieldable", lcore_isyieldable},
    {"sleep", lcore_sleep},
    {"createTimer", lcore_create_timer},
    {"createFuture", lcore_create_future},
    {"createEvent", lcore_create_event},
    {"createQueue", lcore_create_queue},
    {NULL, NULL},
};

LUAMOD_API int luaopen_core(lua_State *L) {
    luaL_newlib(L, lcore_funcs);
    lcore_timer_createmeta(L);
    lcore_future_createmeta(L);
    lcore_event_createmeta(L);
    lcore_queue_createmeta(L);
    return 1;
}
