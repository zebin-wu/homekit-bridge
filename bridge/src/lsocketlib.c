// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/net/socket.h>
#include <HAPBase.h>
#include <HAPLog.h>

#include "lc.h"
#include "app_int.h"

#define LUA_SOCKET_OBJECT_NAME "Socket*"

typedef struct {
    pal_socket_obj *socket;
} lsocket_obj;

static const HAPLogObject lsocket_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lsocket",
};

static const char *lsocket_type_strs[] = {
    "TCP",
    "UDP",
    NULL,
};

static const char *lsocket_af_strs[] = {
    "",
    "IPV4",
    "IPV6",
    NULL,
};

static int lsocket_create(lua_State *L) {
    pal_socket_type type = luaL_checkoption(L, 1, NULL, lsocket_type_strs);
    pal_addr_family af = luaL_checkoption(L, 2, NULL, lsocket_af_strs);

    lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
    luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);

    obj->socket = pal_socket_create(type, af);
    if (luai_unlikely(!obj->socket)) {
        luaL_error(L, "failed to create socket object");
    }

    return 1;
}

static lsocket_obj *lsocket_obj_get(lua_State *L, int idx) {
    lsocket_obj *obj = luaL_checkudata(L, idx, LUA_SOCKET_OBJECT_NAME);
    if (luai_unlikely(!obj->socket)) {
        luaL_error(L, "attemp to use a destroyed socket");
    }
    return obj;
}

static int lsocket_obj_settimeout(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer ms = luaL_checkinteger(L, 2);
    luaL_argcheck(L, ms >= 0 && ms <= UINT32_MAX, 2, "ms out of range");

    pal_socket_set_timeout(obj->socket, ms);

    return 0;
}

static int lsocket_obj_enablebroadcast(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    pal_socket_err err = pal_socket_enable_broadcast(obj->socket);
    if (luai_unlikely(err != PAL_SOCKET_ERR_OK)) {
        luaL_error(L, pal_socket_get_error_str(err));
    }
    return 0;
}

static int lsocket_obj_bind(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    luaL_argcheck(L, (port >= 0) && (port <= 65535), 3, "port out of range");

    pal_socket_err err = pal_socket_bind(obj->socket, addr, port);
    if (luai_unlikely(err != PAL_SOCKET_ERR_OK)) {
        luaL_error(L, pal_socket_get_error_str(err));
    }
    return 0;
}

static int lsocket_obj_listen(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer backlog = luaL_checkinteger(L, 2);

    pal_socket_err err = pal_socket_listen(obj->socket, backlog);
    if (luai_unlikely(err != PAL_SOCKET_ERR_OK)) {
        luaL_error(L, pal_socket_get_error_str(err));
    }

    return 0;
}

static void lsocket_accepted_cb(pal_socket_obj *o, pal_socket_err err, pal_socket_obj *new_o,
    const char *addr, uint16_t port, void *arg) {
    lua_State *co = arg;
    lua_State *L = lc_getmainthread(co);
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, port);
    lua_pushlightuserdata(co, (void *)addr);
    lua_pushlightuserdata(co, new_o);
    lua_pushinteger(co, err);
    status = lc_resume(co, L, 4, &nres);  // stack <..., port, addr, new_o, err>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshaccept(lua_State *L, int status, lua_KContext extra) {
    pal_socket_err err = lua_tointeger(L, -1);
    pal_socket_obj *new_o = lua_touserdata(L, -2);
    const char *addr = lua_touserdata(L, -3);
    uint16_t port = lua_tointeger(L, -4);
    lua_pop(L, 4);

    switch (err) {
    case PAL_SOCKET_ERR_OK: {
        lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
        luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);
        obj->socket = new_o;
        lua_pushstring(L, addr);
        lua_pushinteger(L, port);
        return 3;
    }
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_accept(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    pal_socket_obj *new_o = NULL;

    char addr[64];
    uint16_t port;
    pal_socket_err err = pal_socket_accept(obj->socket, &new_o, addr, sizeof(addr), &port, lsocket_accepted_cb, L);
    switch (err) {
    case PAL_SOCKET_ERR_OK: {
        lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
        luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);
        obj->socket = new_o;
        lua_pushstring(L, addr);
        lua_pushinteger(L, port);
        return 3;
    }
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, (lua_KContext)obj, finshaccept);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static void lsocket_connected_cb(pal_socket_obj *o, pal_socket_err err, void *arg) {
    lua_State *co = arg;
    lua_State *L = lc_getmainthread(co);
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    status = lc_resume(co, L, 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshconnect(lua_State *L, int status, lua_KContext extra) {
    pal_socket_err err = lua_tointeger(L, -1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        break;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, extra, finshconnect);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_connect(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    luaL_argcheck(L, (port >= 0) && (port <= 65535), 3, "port out of range");
    lua_pushinteger(L, pal_socket_connect(obj->socket, addr,
        port, lsocket_connected_cb, L));
    return finshconnect(L, 1, (lua_KContext)obj);
}

static void lsocket_sent_cb(pal_socket_obj *o, pal_socket_err err, size_t sent_len, void *arg) {
    lua_State *co = arg;
    lua_State *L = lc_getmainthread(co);
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    lua_pushinteger(co, sent_len);
    status = lc_resume(co, L, 2, &nres);  // stack <..., sent_len, err>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshsend(lua_State *L, int status, lua_KContext extra) {
    bool all = (bool)extra;
    pal_socket_err err = lua_tointeger(L, -2);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        return all ? 0 : 1;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, extra, finshsend);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_sent_int(lua_State *L, bool all) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    size_t sent_len = len;
    lua_pushinteger(L, pal_socket_send(obj->socket, data, &sent_len, all, lsocket_sent_cb, L));
    lua_pushinteger(L, sent_len);
    return finshsend(L, 2, (lua_KContext)all);
}

static int lsocket_obj_send(lua_State *L) {
    return lsocket_obj_sent_int(L, false);
}

static int lsocket_obj_sendall(lua_State *L) {
    return lsocket_obj_sent_int(L, true);
}

static int lsocket_obj_sendto(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    const char *addr = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);
    luaL_argcheck(L, (port >= 0) && (port <= 65535), 4, "port out of range");

    size_t sent_len = len;
    lua_pushinteger(L, pal_socket_sendto(obj->socket, data, &sent_len, addr, port, false, lsocket_sent_cb, L));
    lua_pushinteger(L, sent_len);
    return finshsend(L, 0, (lua_KContext)false);
}

static void lsocket_recved_cb(pal_socket_obj *o, pal_socket_err err,
    const char *addr, uint16_t port, void *data, size_t len, void *arg) {
    lua_State *co = arg;
    lua_State *L = lc_getmainthread(co);
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushlightuserdata(co, data);
    lua_pushinteger(co, len);
    lua_pushlightuserdata(co, (void *)addr);
    lua_pushinteger(co, port);
    lua_pushinteger(co, err);

    status = lc_resume(co, L, 5, &nres);  // stack <..., data, len, addr, port, err>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshrecv(lua_State *L, int status, lua_KContext extra) {
    bool isrecvfrom = (bool)extra;
    pal_socket_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    switch (err) {
    case PAL_SOCKET_ERR_OK: {
        const char *data = lua_touserdata(L, -4);
        size_t len = lua_tointeger(L, -3);
        const char *addr = lua_touserdata(L, -2);
        uint16_t port = lua_tointeger(L, -1);
        lua_pop(L, 4);
        lua_pushlstring(L, data, len);
        if (!isrecvfrom) {
            return 1;
        }
        lua_pushstring(L, addr);
        lua_pushinteger(L, port);
        return 3;
    } break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_recv(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer maxlen = luaL_checkinteger(L, 2);
    luaL_argcheck(L, maxlen >= 0 && maxlen <= UINT32_MAX, 2, "maxlen out of range");

    char buf[maxlen];
    size_t len = maxlen;
    pal_socket_err err = pal_socket_recv(obj->socket, buf, &len, lsocket_recved_cb, L);
    switch (err) {
    case PAL_SOCKET_ERR_OK:
        lua_pushlstring(L, buf, len);
        return 1;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        return lua_yieldk(L, 0, (lua_KContext)false, finshrecv);
    default:
        lua_pushstring(L, pal_socket_get_error_str(err));
        return lua_error(L);
    }
}

static int lsocket_obj_recvfrom(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer maxlen = luaL_checkinteger(L, 2);
    luaL_argcheck(L, maxlen >= 0 && maxlen <= UINT32_MAX, 2, "maxlen out of range");

    char addr[64];
    uint16_t port;
    char buf[maxlen];
    size_t len = maxlen;
    pal_socket_err err = pal_socket_recvfrom(obj->socket, buf, &len, addr, sizeof(addr),
        &port, lsocket_recved_cb, L);
    switch (err) {
    case PAL_SOCKET_ERR_OK:
        lua_pushlstring(L, buf, len);
        lua_pushstring(L, addr);
        lua_pushinteger(L, port);
        return 3;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        return lua_yieldk(L, 0, (lua_KContext)true, finshrecv);
    default:
        lua_pushstring(L, pal_socket_get_error_str(err));
        return lua_error(L);
    }
}

static int lsocket_obj_readable(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_pushboolean(L, pal_socket_readable(obj->socket));
    return 1;
}

static int lsocket_obj_destroy(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    pal_socket_destroy(obj->socket);
    obj->socket = NULL;
    return 0;
}

static int lsocket_obj_gc(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    if (obj->socket) {
        pal_socket_destroy(obj->socket);
        obj->socket = NULL;
    }
    return 0;
}

static int lsocket_obj_tostring(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    if (obj->socket) {
        lua_pushfstring(L, "socket (%p)", obj->socket);
    } else {
        lua_pushliteral(L, "socket (destroyed)");
    }
    return 1;
}

static const luaL_Reg lsocket_funcs[] = {
    {"create", lsocket_create},
    {NULL, NULL},
};

/*
 * methods for socket object
 */
static const luaL_Reg lsocket_obj_meth[] = {
    {"settimeout", lsocket_obj_settimeout},
    {"enablebroadcast", lsocket_obj_enablebroadcast},
    {"bind", lsocket_obj_bind},
    {"listen", lsocket_obj_listen},
    {"accept", lsocket_obj_accept},
    {"connect", lsocket_obj_connect},
    {"send", lsocket_obj_send},
    {"sendall", lsocket_obj_sendall},
    {"sendto", lsocket_obj_sendto},
    {"recv", lsocket_obj_recv},
    {"recvfrom", lsocket_obj_recvfrom},
    {"readable", lsocket_obj_readable},
    {"destroy", lsocket_obj_destroy},
    {NULL, NULL}
};

/*
 * metamethods for socket object
 */
static const luaL_Reg lsocket_obj_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lsocket_obj_gc},
    {"__close", lsocket_obj_gc},
    {"__tostring", lsocket_obj_tostring},
    {NULL, NULL}
};

static void lsocket_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_SOCKET_OBJECT_NAME);  /* metatable for Socket* */
    luaL_setfuncs(L, lsocket_obj_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lsocket_obj_meth);  /* create method table */
    luaL_setfuncs(L, lsocket_obj_meth, 0);  /* add Socket* methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_socket(lua_State *L) {
    luaL_newlib(L, lsocket_funcs);
    lsocket_createmeta(L);
    return 1;
}
