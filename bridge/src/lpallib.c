// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <lauxlib.h>
#include <pal/board.h>
#include <pal/net/udp.h>
#include <HAPBase.h>
#include <HAPLog.h>
#include <HAPPlatformTimer.h>

#include "app_int.h"
#include "lc.h"
#include "lpallib.h"

static const HAPLogObject lpal_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lpal",
};

static int lpal_board_getManufacturer(lua_State *L) {
    lua_pushstring(L, pal_board_get_manufacturer());
    return 1;
}

static int lpal_board_getModel(lua_State *L) {
    lua_pushstring(L, pal_board_get_model());
    return 1;
}

static int lpal_board_getSerialNumber(lua_State *L) {
    lua_pushstring(L, pal_board_get_serial_number());
    return 1;
}

static int lpal_board_getFirmwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_firmware_version());
    return 1;
}

static int lpal_board_getHardwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_hardware_version());
    return 1;
}

static const luaL_Reg lpal_board_funcs[] = {
    {"getManufacturer", lpal_board_getManufacturer},
    {"getModel", lpal_board_getModel},
    {"getSerialNumber", lpal_board_getSerialNumber},
    {"getFirmwareVersion", lpal_board_getFirmwareVersion},
    {"getHardwareVersion", lpal_board_getHardwareVersion},
    {NULL, NULL},
};

LUAMOD_API int luaopen_pal_board(lua_State *L) {
    luaL_newlib(L, lpal_board_funcs);
    return 1;
}

#define LUA_TIMER_HANDLE_NAME "TimerHandle"

typedef struct {
    lua_State *L;
    HAPPlatformTimerRef timer;
    int argc;
    int ref_ids[1]; /* ref_ids[0] is cb ref id */
} lpal_timer_handle;

#define LPAL_TIMER_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_TIMER_HANDLE_NAME)

static void lpal_timer_handle_reset(lpal_timer_handle *handle) {
    lc_unref(handle->L, handle->ref_ids[0]);
    handle->ref_ids[0] = LUA_REFNIL;

    for (int i = 1; i <= handle->argc; i++) {
        lc_unref(handle->L, handle->ref_ids[i]);
        handle->ref_ids[i] = LUA_REFNIL;
    }
    handle->argc = 0;
    handle->L = NULL;
}

static void lpal_timer_cb(HAPPlatformTimerRef timer, void* _Nullable context) {
    lpal_timer_handle *handle = context;
    lua_State *L = handle->L;

    handle->timer = 0;

    if (!lc_push_ref(L, handle->ref_ids[0])) {
        HAPLogError(&lpal_log, "%s: Can't get lua function.", __func__);
        goto end;
    }
    for (int i = 1; i <= handle->argc; i++) {
        if (!lc_push_ref(L, handle->ref_ids[i])) {
            HAPLogError(&lpal_log, "%s: Can't get #arg %d set in timer.create().",
                __func__, i + 2);
            goto end;
        }
    }

    if (lua_pcall(L, handle->argc, 0, 0)) {
        HAPLogError(&lpal_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
end:
    lpal_timer_handle_reset(handle);
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int lpal_timer_create(lua_State *L) {
    int argc = lua_gettop(L) - 2;
    lua_Integer ms = luaL_checkinteger(L, 1);
    if (ms < 0) {
        goto err;
    }
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lpal_timer_handle *handle = lua_newuserdata(L, sizeof(lpal_timer_handle) + sizeof(int) * argc);
    luaL_setmetatable(L, LUA_TIMER_HANDLE_NAME);
    HAPError err = HAPPlatformTimerRegister(&handle->timer,
        (HAPTime)ms + HAPPlatformClockGetCurrent(), lpal_timer_cb, handle);
    if (err != kHAPError_None) {
        goto err;
    }
    handle->L = L;
    handle->argc = argc;
    handle->ref_ids[0] = lc_ref(L, 2);
    for (int i = 1; i <= argc; i++) {
        handle->ref_ids[i] = lc_ref(L, i + 2);
    }
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int lpal_timer_handle_destroy(lua_State *L) {
    lpal_timer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    if (!handle->timer) {
        luaL_error(L, "attempt to use a destroyed timer");
    }
    lpal_timer_handle_reset(handle);
    HAPPlatformTimerDeregister(handle->timer);
    handle->timer = 0;
    return 0;
}

static int lpal_timer_handle_gc(lua_State *L) {
    lpal_timer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    lpal_timer_handle_reset(handle);
    if (handle->timer) {
        HAPPlatformTimerDeregister(handle->timer);
        handle->timer = 0;
    }
    return 0;
}

static int lpal_timer_handle_tostring(lua_State *L) {
    lpal_timer_handle *handle = LPAL_TIMER_GET_HANDLE(L, 1);
    if (handle->timer) {
        lua_pushfstring(L, "timer (%p)", handle->timer);
    } else {
        lua_pushliteral(L, "timer (destroyed)");
    }
    return 1;
}

static const luaL_Reg lpal_timer_funcs[] = {
    {"create", lpal_timer_create},
    {NULL, NULL},
};

/*
 * metamethods for TimerHandle
 */
static const luaL_Reg lpal_timer_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lpal_timer_handle_gc},
    {"__close", lpal_timer_handle_gc},
    {"__tostring", lpal_timer_handle_tostring},
    {NULL, NULL}
};

/*
 * methods for UdpHandle
 */
static const luaL_Reg lpal_timer_handle_meth[] = {
    {"destroy", lpal_timer_handle_destroy},
    {NULL, NULL},
};

static void lpal_timer_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_TIMER_HANDLE_NAME);  /* metatable for UDP handle */
    luaL_setfuncs(L, lpal_timer_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lpal_timer_handle_meth);  /* create method table */
    luaL_setfuncs(L, lpal_timer_handle_meth, 0);  /* add udp handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_pal_timer(lua_State *L) {
    luaL_newlib(L, lpal_timer_funcs);
    lpal_timer_createmeta(L);
    return 1;
}

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
} lpal_net_udp_handle;

#define LPAL_NET_UDP_GET_HANDLE(L, idx) \
    luaL_checkudata(L, idx, LUA_UDP_HANDLE_NAME)

static pal_net_udp *lpal_net_udp_to_pcb(lua_State *L, int idx) {
    lpal_net_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, idx);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    return handle->udp;
}

static int lpal_net_udp_open(lua_State *L) {
    pal_net_domain domain = PAL_NET_DOMAIN_COUNT;
    const char *str = luaL_checkstring(L, 1);
    for (size_t i = 0; i < HAPArrayCount(net_domain_strs); i++) {
        if (!strcmp(net_domain_strs[i], str)) {
            domain = i;
        }
    }
    if (domain == PAL_NET_DOMAIN_COUNT) {
        goto err;
    }

    lpal_net_udp_handle *handle = lua_newuserdata(L, sizeof(lpal_net_udp_handle));
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

static int lhap_net_udp_handle_enable_broadcast(lua_State *L) {
    pal_net_udp *udp = lpal_net_udp_to_pcb(L, 1);
    lua_pushboolean(L,
        pal_net_udp_enable_broadcast(udp) == PAL_NET_ERR_OK ? true : false);
    return 1;
}

static bool lpal_net_port_valid(lua_Integer port) {
    return ((port >= 0) && (port <= 65535));
}

static int lpal_net_udp_handle_bind(lua_State *L) {
    pal_net_udp *udp = lpal_net_udp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lpal_net_port_valid(port)) {
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

static int lpal_net_udp_handle_connect(lua_State *L) {
    pal_net_udp *udp = lpal_net_udp_to_pcb(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lpal_net_port_valid(port)) {
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

static int lpal_net_udp_handle_send(lua_State *L) {
    pal_net_udp *udp = lpal_net_udp_to_pcb(L, 1);
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

static int lpal_net_udp_handle_sendto(lua_State *L) {
    pal_net_udp *udp = lpal_net_udp_to_pcb(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    const char *addr = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);
    if (!lpal_net_port_valid(port)) {
        goto err;
    }
    pal_net_err err = pal_net_udp_sendto(udp, data, len, addr, (uint16_t)port);
    if (err != PAL_NET_ERR_OK) {
        goto err;
    }
    return 0;

err:
    lua_pushboolean(L, false);
    return 1;
}

static void lpal_net_udp_recv_cb(pal_net_udp *udp, void *data, size_t len,
    const char *from_addr, uint16_t from_port, void *arg) {
    lpal_net_udp_handle *handle = arg;
    if (!lc_push_ref(handle->L, handle->ref_ids.recv_cb)) {
        HAPLogError(&lpal_log, "%s: Can't get lua function.", __func__);
        return;
    }
    lua_pushlstring(handle->L, (const char *)data, len);
    lua_pushstring(handle->L, from_addr);
    lua_pushinteger(handle->L, from_port);
    bool has_arg = lc_push_ref(handle->L, handle->ref_ids.recv_arg);
    if (lua_pcall(handle->L, has_arg ? 4 : 3, 0, 0)) {
        HAPLogError(&lpal_log, "%s: %s", __func__, lua_tostring(handle->L, -1));
    }
    lua_settop(handle->L, 0);
    lc_collectgarbage(handle->L);
}

static int lpal_net_udp_handle_set_recv_cb(lua_State *L) {
    lpal_net_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
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
        pal_net_udp_set_recv_cb(handle->udp, lpal_net_udp_recv_cb, handle);
    } else {
        pal_net_udp_set_recv_cb(handle->udp, NULL, NULL);
    }
    return 0;
}

static void lpal_net_udp_err_cb(pal_net_udp *udp, pal_net_err err, void *arg) {
    lpal_net_udp_handle *handle = arg;
    if (!lc_push_ref(handle->L, handle->ref_ids.err_cb)) {
        HAPLogError(&lpal_log, "%s: Can't get lua function.", __func__);
        return;
    }
    bool has_arg = lc_push_ref(handle->L, handle->ref_ids.err_arg);
    if (lua_pcall(handle->L, has_arg ? 1 : 0, 0, 0)) {
        HAPLogError(&lpal_log, "%s: %s", __func__, lua_tostring(handle->L, -1));
    }
    lua_settop(handle->L, 0);
    lc_collectgarbage(handle->L);
}

static int lpal_net_udp_handle_set_err_cb(lua_State *L) {
    lpal_net_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
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
        pal_net_udp_set_err_cb(handle->udp, lpal_net_udp_err_cb, handle);
    } else {
        pal_net_udp_set_err_cb(handle->udp, NULL, NULL);
    }
    return 0;
}

static void lhap_net_udp_handle_reset(lpal_net_udp_handle *handle) {
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

static int lpal_net_udp_handle_close(lua_State *L) {
    lpal_net_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (!handle->udp) {
        luaL_error(L, "attempt to use a closed handle");
    }
    lhap_net_udp_handle_reset(handle);
    return 0;
}

static int lpal_net_udp_handle_gc(lua_State *L) {
    lhap_net_udp_handle_reset(LPAL_NET_UDP_GET_HANDLE(L, 1));
    return 0;
}

static int lpal_net_udp_handle_tostring(lua_State *L) {
    lpal_net_udp_handle *handle = LPAL_NET_UDP_GET_HANDLE(L, 1);
    if (handle->udp) {
        lua_pushfstring(L, "UDP handle (%p)", handle->udp);
    } else {
        lua_pushliteral(L, "UDP handle (closed)");
    }
    return 1;
}

static const luaL_Reg lpal_net_udp_funcs[] = {
    {"open", lpal_net_udp_open},
    {NULL, NULL},
};

/*
 * methods for UdpHandle
 */
static const luaL_Reg lpal_net_udp_handle_meth[] = {
    {"enableBroadcast", lhap_net_udp_handle_enable_broadcast},
    {"bind", lpal_net_udp_handle_bind},
    {"connect", lpal_net_udp_handle_connect},
    {"send", lpal_net_udp_handle_send},
    {"sendto", lpal_net_udp_handle_sendto},
    {"setRecvCb", lpal_net_udp_handle_set_recv_cb},
    {"setErrCb", lpal_net_udp_handle_set_err_cb},
    {"close", lpal_net_udp_handle_close},
    {NULL, NULL}
};

/*
 * metamethods for UdpHandle
 */
static const luaL_Reg lpal_net_udp_handle_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lpal_net_udp_handle_gc},
    {"__close", lpal_net_udp_handle_gc},
    {"__tostring", lpal_net_udp_handle_tostring},
    {NULL, NULL}
};

static void lpal_net_udp_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_UDP_HANDLE_NAME);  /* metatable for UDP handle */
    luaL_setfuncs(L, lpal_net_udp_handle_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lpal_net_udp_handle_meth);  /* create method table */
    luaL_setfuncs(L, lpal_net_udp_handle_meth, 0);  /* add udp handle methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_pal_net_udp(lua_State *L) {
    luaL_newlib(L, lpal_net_udp_funcs);
    lpal_net_udp_createmeta(L);
    return 1;
}
