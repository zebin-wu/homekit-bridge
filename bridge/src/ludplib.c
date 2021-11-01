// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/udp.h>
#include <HAPBase.h>
#include <HAPLog.h>

#include "lc.h"
#include "app_int.h"

static const HAPLogObject lnet_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "ludp",
};

#define LUA_UDP_HANDLE_NAME "UdpHandle*"

static const char *net_domain_strs[] = PAL_NET_DOMAIN_STRS;

typedef struct {
    pal_udp *udp;
    lua_State *L;
    size_t recv_nargs;
    size_t err_nargs;
} ludp_handle;

#define LPAL_NET_UDP_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_UDP_HANDLE_NAME)

static pal_udp *ludp_to_pcb(lua_State *L, int idx) {
    ludp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, idx);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    return handle->udp;
}

static int ludp_open(lua_State *L) {
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

    ludp_handle *handle = lua_newuserdata(L, sizeof(ludp_handle));
    luaL_setmetatable(L, LUA_UDP_HANDLE_NAME);
    handle->udp = pal_udp_new(domain);
    if (!handle->udp) {
        goto err;
    }
    handle->L = L;
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int ludp_handle_enable_broadcast(lua_State *L) {
    pal_udp_enable_broadcast(ludp_to_pcb(L, 1));
    return 0;
}

static bool ludp_port_valid(lua_Integer port) {
    return ((port >= 0) && (port <= 65535));
}

static int ludp_handle_bind(lua_State *L) {
    pal_udp *udp = ludp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!ludp_port_valid(port)) {
        luaL_argerror(L, 3, "port out of range");
    }
    lua_pushboolean(L,
        pal_udp_bind(udp, addr, (uint16_t)port) == PAL_NET_ERR_OK);
    return 1;
}

static int ludp_handle_connect(lua_State *L) {
    pal_udp *udp = ludp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!ludp_port_valid(port)) {
        luaL_argerror(L, 3, "port out of range");
    }
    lua_pushboolean(L,
        pal_udp_connect(udp, addr, (uint16_t)port) == PAL_NET_ERR_OK);
    return 1;
}

static int ludp_handle_send(lua_State *L) {
    pal_udp *udp = ludp_to_pcb(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    lua_pushboolean(L, pal_udp_send(udp, data, len) == PAL_NET_ERR_OK);
    return 1;
}

static int ludp_handle_sendto(lua_State *L) {
    pal_udp *udp = ludp_to_pcb(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    const char *addr = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);
    if (!ludp_port_valid(port)) {
        luaL_argerror(L, 3, "port out of range");
    }
    lua_pushboolean(L, pal_udp_sendto(udp, data, len, addr, (uint16_t)port) == PAL_NET_ERR_OK);
    return 1;
}

static void ludp_recv_cb(pal_udp *udp, void *data, size_t len,
    const char *from_addr, uint16_t from_port, void *arg) {
    ludp_handle *handle = arg;
    lua_State *L = handle->L;

    HAPAssert(lua_gettop(L) == 0);
    lc_push_traceback(L);

    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &handle->recv_nargs) == LUA_TTABLE);
    lua_geti(L, 2, 1);
    lua_pushlstring(L, (const char *)data, len);
    lua_pushstring(L, from_addr);
    lua_pushinteger(L, from_port);
    for (int i = 2; i <= handle->recv_nargs + 1; i++) {
        lua_geti(L, 2, i);
    }
    lua_remove(L, 2);

    if (lua_pcall(L, handle->recv_nargs + 3, 0, 1)) {
        HAPLogError(&lnet_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ludp_handle_set_recv_cb(lua_State *L) {
    ludp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }

    int n = lua_gettop(L) - 1;
    if (n == 0) {
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->recv_nargs);
        pal_udp_set_recv_cb(handle->udp, NULL, NULL);
        return 0;
    }

    luaL_argexpected(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function");

    lua_createtable(L, n, 0);
    lua_insert(L, 2);
    for (int i = n; i >= 1; i--) {
        lua_seti(L, 2, i);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->recv_nargs);
    handle->recv_nargs = n - 1;
    pal_udp_set_recv_cb(handle->udp, ludp_recv_cb, handle);
    return 0;
}

static void ludp_err_cb(pal_udp *udp, pal_net_err err, void *arg) {
    ludp_handle *handle = arg;
    lua_State *L = handle->L;

    HAPAssert(lua_gettop(L) == 0);
    lc_push_traceback(L);

    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &handle->err_nargs) == LUA_TTABLE);
    for (int i = 1; i <= handle->err_nargs + 1; i++) {
        lua_geti(L, 2, i);
    }
    lua_remove(L, 2);

    if (lua_pcall(L, handle->err_nargs, 0, 1)) {
        HAPLogError(&lnet_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int ludp_handle_set_err_cb(lua_State *L) {
    ludp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }

    int n = lua_gettop(L) - 1;
    if (n == 0) {
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->err_nargs);
        pal_udp_set_err_cb(handle->udp, NULL, NULL);
        return 0;
    }

    luaL_argexpected(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function");

    lua_createtable(L, n, 0);
    lua_insert(L, 2);
    for (int i = n; i >= 1; i--) {
        lua_seti(L, 2, i);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->err_nargs);
    handle->err_nargs = n - 1;
    pal_udp_set_err_cb(handle->udp, ludp_err_cb, handle);
    return 0;
}

static void lhap_net_udp_handle_reset(lua_State *L, ludp_handle *handle) {
    if (!handle->udp) {
        return;
    }
    pal_udp_free(handle->udp);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->recv_nargs);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &handle->err_nargs);
    HAPRawBufferZero(handle, sizeof(*handle));
}

static int ludp_handle_close(lua_State *L) {
    ludp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    lhap_net_udp_handle_reset(L, handle);
    return 0;
}

static int ludp_handle_gc(lua_State *L) {
    lhap_net_udp_handle_reset(L, LPAL_NET_UDP_GET_HANDLE(L, 1));
    return 0;
}

static int ludp_handle_tostring(lua_State *L) {
    ludp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (handle->udp) {
        lua_pushfstring(L, "UDP handle (%p)", handle->udp);
    } else {
        lua_pushliteral(L, "UDP handle (closed)");
    }
    return 1;
}

static const luaL_Reg ludp_funcs[] = {
    {"open", ludp_open},
    {NULL, NULL},
};

/*
 * methods for UdpHandle
 */
static const luaL_Reg ludp_handle_meth[] = {
    {"enableBroadcast", ludp_handle_enable_broadcast},
    {"bind", ludp_handle_bind},
    {"connect", ludp_handle_connect},
    {"send", ludp_handle_send},
    {"sendto", ludp_handle_sendto},
    {"setRecvCb", ludp_handle_set_recv_cb},
    {"setErrCb", ludp_handle_set_err_cb},
    {"close", ludp_handle_close},
    {NULL, NULL}
};

/*
 * metamethods for UdpHandle
 */
static const luaL_Reg ludp_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", ludp_handle_gc},
    {"__close", ludp_handle_gc},
    {"__tostring", ludp_handle_tostring},
    {NULL, NULL}
};

static void ludp_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_UDP_HANDLE_NAME);  /* metatable for UDP handle */
    luaL_setfuncs(L, ludp_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, ludp_handle_meth);  /* create method table */
    luaL_setfuncs(L, ludp_handle_meth, 0);  /* add udp handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_udp(lua_State *L) {
    luaL_newlib(L, ludp_funcs);
    ludp_createmeta(L);
    return 1;
}
