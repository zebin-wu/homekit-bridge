// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/net/dns.h>

#include "app_int.h"
#include "lc.h"

static const char *ldns_family_strs[] = {
    "",
    "IPV4",
    "IPV6",
    NULL
};

static const HAPLogObject ldns_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ldns",
};

typedef struct ldns_resolve_context {
    lua_State *co;
    pal_dns_req_ctx *req;
    HAPPlatformTimerRef timer;
} ldns_resolve_context;

static int ldns_response(lua_State *L) {
    lua_State *co = lua_touserdata(L, 1);
    const char *err = lua_touserdata(L, 2);
    const char *addr = lua_touserdata(L, 3);
    pal_addr_family af = lua_tointeger(L, 4);
    lua_pop(L, 4);

    int narg = 0;
    if (err) {
        narg = 1;
        lua_pushstring(co, err);
    } else {
        HAPAssert(addr);
        narg = 3;
        lua_pushstring(co, addr);
        lua_pushstring(co, ldns_family_strs[af]);
        lua_pushnil(co);
    }
    int status, nres;
    status = lc_resume(co, L, narg, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&ldns_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    return 0;
}

void ldns_response_cb(const char *err, const char *addr, pal_addr_family af, void *arg) {
    ldns_resolve_context *ctx = arg;
    lua_State *co = ctx->co;
    lua_State *L = lc_getmainthread(co);

    if (ctx->timer) {
        HAPPlatformTimerDeregister(ctx->timer);
    }

    HAPAssert(lua_gettop(L) == 0);

    lc_pushtraceback(L);
    lua_pushcfunction(L, ldns_response);
    lua_pushlightuserdata(L, co);
    lua_pushlightuserdata(L, (void *)err);
    lua_pushlightuserdata(L, (void *)addr);
    lua_pushinteger(L, af);
    int status = lua_pcall(L, 4, 0, 1);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&ldns_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void ldns_timeout_timer_cb(HAPPlatformTimerRef timer, void *context) {
    ldns_resolve_context *ctx = context;
    ctx->timer = 0;
    pal_dns_cancel_request(ctx->req);
    ldns_response_cb("resolve timeout", NULL, PAL_ADDR_FAMILY_UNSPEC, ctx);
}

static int finshresolve(lua_State *L, int status, lua_KContext extra) {
    if (lua_isstring(L, -1)) {
        lua_error(L);
    }
    lua_pop(L, 1);
    return 2;
}

static int ldns_resolve(lua_State *L) {
    const char *hostname = luaL_checkstring(L, 1);
    lua_Integer ms = luaL_checkinteger(L, 2);
    luaL_argcheck(L, ms >= 0, 2, "timeout out of range");
    pal_addr_family af = luaL_checkoption(L, 3, "", ldns_family_strs);

    ldns_resolve_context *ctx = lua_newuserdata(L, sizeof(*ctx));
    if (luai_unlikely(HAPPlatformTimerRegister(&ctx->timer,
        HAPPlatformClockGetCurrent() + ms,
        ldns_timeout_timer_cb, ctx) != kHAPError_None)) {
        luaL_error(L, "failed to create a timeout timer");
    }
    ctx->req = pal_dns_start_request(hostname, af, ldns_response_cb, ctx);
    if (luai_unlikely(!ctx->req)) {
        HAPPlatformTimerDeregister(ctx->timer);
        luaL_error(L, "failed to start DNS resolution request");
    }
    ctx->co = L;
    return lua_yieldk(L, 0, 0, finshresolve);
}

static const luaL_Reg ldns_funcs[] = {
    {"resolve", ldns_resolve},
    {NULL, NULL},
};

LUAMOD_API int luaopen_dns(lua_State *L) {
    luaL_newlib(L, ldns_funcs);
    return 1;
}
