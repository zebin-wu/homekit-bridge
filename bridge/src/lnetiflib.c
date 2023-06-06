// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/net_if.h>
#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPBase.h>
#include "app_int.h"
#include "lc.h"

static const HAPLogObject lnetif_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "netif",
};

static const char *lnetif_event_strs[] = {
    "Added",
    "Removed",
    "Up",
    "Down",
    "Ipv4Changed",
    "Ipv6Changed",
    NULL
};

static pal_err lnetif_foreach_func(pal_net_if *netif, void *arg) {
    lua_State *L = arg;

    lua_pushlightuserdata(L, netif);
    lua_seti(L, -2, luaL_len(L, -2) + 1);

    return PAL_ERR_OK;
}

static int lnetif_get_interfaces(lua_State *L) {
    lua_newtable(L);
    pal_err err = pal_net_if_foreach(lnetif_foreach_func, L);
    if (err != PAL_ERR_OK) {
        luaL_error(L, "failed to get interfaces: %s", pal_err_string(err));
    }
    return 1;
}

static int lnetif_find(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    pal_net_if *netif = pal_net_if_find(name);
    if (!netif) {
        luaL_error(L, "netif '%s' not found", name);
    }

    lua_pushlightuserdata(L, netif);
    return 1;
}

static void lnetif_event_cb(pal_net_if *netif, pal_net_if_event event, void *arg) {
    lua_State *co = arg;
    lua_State *L = lc_getmainthread(co);
    int status, nres;

    lua_pushlightuserdata(co, netif);
    status = lc_resume(co, L, 1, &nres);  // stack <..., netif>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lnetif_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finishwait(lua_State *L, int status, lua_KContext extra) {
    return 1;
}

static int lnetif_wait(lua_State *L) {
    pal_net_if_event event = luaL_checkoption(L, 1, NULL, lnetif_event_strs);
    pal_net_if *netif = lua_touserdata(L, 2);

    pal_net_if_register_event_callback(netif, event, lnetif_event_cb, L);
    return lua_yieldk(L, 0, 0, finishwait);
}

static int lnetif_is_up(lua_State *L) {
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "not a lightuserdata");
    pal_net_if *netif = lua_touserdata(L, 1);

    bool isup;
    pal_err err = pal_net_if_is_up(netif, &isup);
    if (err != PAL_ERR_OK) {
        luaL_error(L, "failed to get up/down: %s", pal_err_string(err));
    }
    lua_pushboolean(L, isup);
    return 1;
}

static int lnetif_get_ipv4_addr(lua_State *L) {
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "not a lightuserdata");
    pal_net_if *netif = lua_touserdata(L, 1);

    pal_net_addr addr;
    pal_err err = pal_net_if_get_ipv4_addr(netif, &addr);
    if (err != PAL_ERR_OK) {
        luaL_error(L, "failed to get IPv4 addr: %s", pal_err_string(err));
    }
    char buf[PAL_NET_ADDR_STR_LEN];
    lua_pushstring(L, pal_net_addr_get_string(&addr, buf, sizeof(buf)));
    return 1;
}

static pal_err lnetif_ipv6_foreach_func(pal_net_if *netif, pal_net_addr *addr, void *arg) {
    lua_State *L = arg;

    char buf[PAL_NET_ADDR_STR_LEN];
    lua_pushstring(L, pal_net_addr_get_string(addr, buf, sizeof(buf)));
    lua_seti(L, -2, luaL_len(L, -2) + 1);

    return PAL_ERR_OK;
}

static int lnetif_get_ipv6_addrs(lua_State *L) {
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "not a lightuserdata");
    pal_net_if *netif = lua_touserdata(L, 1);

    lua_newtable(L);
    pal_err err = pal_net_if_ipv6_addr_foreach(netif, lnetif_ipv6_foreach_func, L);
    if (err != PAL_ERR_OK) {
        luaL_error(L, "failed to get IPv6 addrs: %s", pal_err_string(err));
    }
    return 0;
}

static const luaL_Reg lnetif_funcs[] = {
    {"getInterfaces", lnetif_get_interfaces},
    {"find", lnetif_find},
    {"wait", lnetif_wait},
    {"isUp", lnetif_is_up},
    {"getIpv4Addr", lnetif_get_ipv4_addr},
    {"getIpv6Addrs", lnetif_get_ipv6_addrs},
    {NULL, NULL},
};

LUAMOD_API int luaopen_netif(lua_State *L) {
    luaL_newlib(L, lnetif_funcs);
    return 1;
}
