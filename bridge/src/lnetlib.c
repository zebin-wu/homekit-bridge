// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdbool.h>
#include <lauxlib.h>
#include <pal/net/udp.h>
#include <HAPBase.h>
#include <HAPLog.h>

#include "lc.h"
#include "app_int.h"
#include "lnetlib.h"

static const HAPLogObject lnet_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lnet",
};

#define LUA_UDP_HANDLE_NAME "UdpHandle"

static const char *net_domain_strs[] = PAL_NET_DOMAIN_STRS;

typedef struct {
    pal_net_udp *udp;
    lua_State *L;
    struct {
        int recv_cb;
        int recv_arg;
        int err_cb;
        int err_arg;
    } ref_ids;
} lnet_udp_handle;

#define LPAL_NET_UDP_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_UDP_HANDLE_NAME)

static pal_net_udp *lnet_udp_to_pcb(lua_State *L, int idx) {
    lnet_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, idx);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    return handle->udp;
}

static int lnet_udp_open(lua_State *L) {
    pal_net_domain domain = PAL_NET_DOMAIN_COUNT;
    const char *str = luaL_checkstring(L, 1);
    for (size_t i = 0; i < HAPArrayCount(net_domain_strs); i++) {
        if (HAPStringAreEqual(net_domain_strs[i], str)) {
            domain = i;
        }
    }
    if (domain == PAL_NET_DOMAIN_COUNT) {
        goto err;
    }

    lnet_udp_handle *handle = lua_newuserdata(L, sizeof(lnet_udp_handle));
    luaL_setmetatable(L, LUA_UDP_HANDLE_NAME);
    handle->udp = pal_net_udp_new(domain);
    if (!handle->udp) {
        goto err;
    }
    handle->L = L;
    handle->ref_ids.recv_cb = LUA_REFNIL;
    handle->ref_ids.recv_arg = LUA_REFNIL;
    handle->ref_ids.err_cb = LUA_REFNIL;
    handle->ref_ids.err_arg = LUA_REFNIL;
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int lnet_udp_handle_enable_broadcast(lua_State *L) {
    pal_net_udp *udp = lnet_udp_to_pcb(L, 1);
    lua_pushboolean(L,
        pal_net_udp_enable_broadcast(udp) == PAL_NET_ERR_OK ? true : false);
    return 1;
}

static bool lnet_port_valid(lua_Integer port) {
    return ((port >= 0) && (port <= 65535));
}

static int lnet_udp_handle_bind(lua_State *L) {
    pal_net_udp *udp = lnet_udp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lnet_port_valid(port)) {
        goto err;
    }
    pal_net_err err = pal_net_udp_bind(udp, addr, (uint16_t)port);
    if (err != PAL_NET_ERR_OK) {
        goto err;
    }
    lua_pushboolean(L, true);
    return 1;

err:
    lua_pushboolean(L, false);
    return 1;
}

static int lnet_udp_handle_connect(lua_State *L) {
    pal_net_udp *udp = lnet_udp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lnet_port_valid(port)) {
        goto err;
    }
    pal_net_err err = pal_net_udp_connect(udp, addr, (uint16_t)port);
    if (err != PAL_NET_ERR_OK) {
        goto err;
    }
    lua_pushboolean(L, true);
    return 1;

err:
    lua_pushboolean(L, false);
    return 1;
}

static int lnet_udp_handle_send(lua_State *L) {
    pal_net_udp *udp = lnet_udp_to_pcb(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    pal_net_err err = pal_net_udp_send(udp, data, len);
    if (err != PAL_NET_ERR_OK) {
        goto err;
    }
    lua_pushboolean(L, true);
    return 1;

err:
    lua_pushboolean(L, false);
    return 1;
}

static int lnet_udp_handle_sendto(lua_State *L) {
    pal_net_udp *udp = lnet_udp_to_pcb(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    const char *addr = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);
    if (!lnet_port_valid(port)) {
        goto err;
    }
    pal_net_err err = pal_net_udp_sendto(udp, data, len, addr, (uint16_t)port);
    if (err != PAL_NET_ERR_OK) {
        goto err;
    }
    lua_pushboolean(L, true);
    return 1;

err:
    lua_pushboolean(L, false);
    return 1;
}

static void lnet_udp_recv_cb(pal_net_udp *udp, void *data, size_t len,
    const char *from_addr, uint16_t from_port, void *arg) {
    lnet_udp_handle *handle = arg;
    if (!lc_push_ref(handle->L, handle->ref_ids.recv_cb)) {
        HAPLogError(&lnet_log, "%s: Can't get lua function.", __func__);
        return;
    }
    lua_pushlstring(handle->L, (const char *)data, len);
    lua_pushstring(handle->L, from_addr);
    lua_pushinteger(handle->L, from_port);
    bool has_arg = lc_push_ref(handle->L, handle->ref_ids.recv_arg);
    if (lua_pcall(handle->L, has_arg ? 4 : 3, 0, 0)) {
        HAPLogError(&lnet_log, "%s: %s", __func__, lua_tostring(handle->L, -1));
    }
    lua_settop(handle->L, 0);
    lc_collectgarbage(handle->L);
}

static int lnet_udp_handle_set_recv_cb(lua_State *L) {
    lnet_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }

    lc_unref(L, handle->ref_ids.recv_cb);
    if (lua_isnil(L, 2)) {
        handle->ref_ids.recv_cb = LUA_REFNIL;
    } else {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        handle->ref_ids.recv_cb = lc_ref(L, 2);
    }

    handle->ref_ids.recv_cb = lc_ref(L, 2);
    lc_unref(L, handle->ref_ids.recv_arg);
    if (lua_isnil(L, 3)) {
        handle->ref_ids.recv_arg = LUA_REFNIL;
    } else {
        handle->ref_ids.recv_arg = lc_ref(L, 3);
    }

    if (handle->ref_ids.recv_cb) {
        pal_net_udp_set_recv_cb(handle->udp, lnet_udp_recv_cb, handle);
    } else {
        pal_net_udp_set_recv_cb(handle->udp, NULL, NULL);
    }
    return 0;
}

static void lnet_udp_err_cb(pal_net_udp *udp, pal_net_err err, void *arg) {
    lnet_udp_handle *handle = arg;
    if (!lc_push_ref(handle->L, handle->ref_ids.err_cb)) {
        HAPLogError(&lnet_log, "%s: Can't get lua function.", __func__);
        return;
    }
    bool has_arg = lc_push_ref(handle->L, handle->ref_ids.err_arg);
    if (lua_pcall(handle->L, has_arg ? 1 : 0, 0, 0)) {
        HAPLogError(&lnet_log, "%s: %s", __func__, lua_tostring(handle->L, -1));
    }
    lua_settop(handle->L, 0);
    lc_collectgarbage(handle->L);
}

static int lnet_udp_handle_set_err_cb(lua_State *L) {
    lnet_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }

    lc_unref(L, handle->ref_ids.err_cb);
    if (lua_isnil(L, 2)) {
        handle->ref_ids.err_cb = LUA_REFNIL;
    } else {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        handle->ref_ids.err_cb = lc_ref(L, 2);
    }

    handle->ref_ids.err_arg = lc_ref(L, 2);
    lc_unref(L, handle->ref_ids.err_arg);
    if (lua_isnil(L, 3)) {
        handle->ref_ids.err_arg = LUA_REFNIL;
    } else {
        handle->ref_ids.err_arg = lc_ref(L, 3);
    }

    if (handle->ref_ids.err_cb) {
        pal_net_udp_set_err_cb(handle->udp, lnet_udp_err_cb, handle);
    } else {
        pal_net_udp_set_err_cb(handle->udp, NULL, NULL);
    }
    return 0;
}

static void lhap_net_udp_handle_reset(lnet_udp_handle *handle) {
    if (handle->udp) {
        pal_net_udp_free(handle->udp);
        handle->udp = NULL;
    }
    lc_unref(handle->L, handle->ref_ids.recv_cb);
    handle->ref_ids.recv_cb = LUA_REFNIL;
    lc_unref(handle->L, handle->ref_ids.recv_arg);
    handle->ref_ids.recv_arg = LUA_REFNIL;
    lc_unref(handle->L, handle->ref_ids.err_cb);
    handle->ref_ids.err_cb = LUA_REFNIL;
    lc_unref(handle->L, handle->ref_ids.err_arg);
    handle->ref_ids.err_arg = LUA_REFNIL;
    handle->L = NULL;
}

static int lnet_udp_handle_close(lua_State *L) {
    lnet_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    lhap_net_udp_handle_reset(handle);
    return 0;
}

static int lnet_udp_handle_gc(lua_State *L) {
    lhap_net_udp_handle_reset(LPAL_NET_UDP_GET_HANDLE(L, 1));
    return 0;
}

static int lnet_udp_handle_tostring(lua_State *L) {
    lnet_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (handle->udp) {
        lua_pushfstring(L, "UDP handle (%p)", handle->udp);
    } else {
        lua_pushliteral(L, "UDP handle (closed)");
    }
    return 1;
}

static const luaL_Reg lnet_udp_funcs[] = {
    {"open", lnet_udp_open},
    {NULL, NULL},
};

/*
 * methods for UdpHandle
 */
static const luaL_Reg lnet_udp_handle_meth[] = {
    {"enableBroadcast", lnet_udp_handle_enable_broadcast},
    {"bind", lnet_udp_handle_bind},
    {"connect", lnet_udp_handle_connect},
    {"send", lnet_udp_handle_send},
    {"sendto", lnet_udp_handle_sendto},
    {"setRecvCb", lnet_udp_handle_set_recv_cb},
    {"setErrCb", lnet_udp_handle_set_err_cb},
    {"close", lnet_udp_handle_close},
    {NULL, NULL}
};

/*
 * metamethods for UdpHandle
 */
static const luaL_Reg lnet_udp_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lnet_udp_handle_gc},
    {"__close", lnet_udp_handle_gc},
    {"__tostring", lnet_udp_handle_tostring},
    {NULL, NULL}
};

static void lnet_udp_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_UDP_HANDLE_NAME);  /* metatable for UDP handle */
    luaL_setfuncs(L, lnet_udp_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lnet_udp_handle_meth);  /* create method table */
    luaL_setfuncs(L, lnet_udp_handle_meth, 0);  /* add udp handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_net_udp(lua_State *L) {
    luaL_newlib(L, lnet_udp_funcs);
    lnet_udp_createmeta(L);
    return 1;
}
