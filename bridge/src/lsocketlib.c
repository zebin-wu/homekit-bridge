// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/socket.h>
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

static int lsocket_create(lua_State *L) {
    pal_socket_type type = luaL_checkoption(L, 1, NULL, (const char *[]) {
        [PAL_SOCKET_TYPE_TCP] = "TCP",
        [PAL_SOCKET_TYPE_UDP] = "UDP"
    });
    pal_socket_domain domain = luaL_checkoption(L, 2, NULL, (const char *[]) {
        [PAL_SOCKET_DOMAIN_INET] = "INET",
        [PAL_SOCKET_DOMAIN_INET6] = "INET6"
    });

    lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
    luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);

    obj->socket = pal_socket_create(type, domain);
    if (!obj->socket) {
        luaL_error(L, "failed to create socket object");
    }

    return 1;
}

static lsocket_obj *lsocket_obj_get(lua_State *L, int idx) {
    lsocket_obj *obj = luaL_checkudata(L, idx, LUA_SOCKET_OBJECT_NAME);
    if (!obj->socket) {
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
    if (err != PAL_SOCKET_ERR_OK) {
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
    if (err != PAL_SOCKET_ERR_OK) {
        luaL_error(L, pal_socket_get_error_str(err));
    }
    return 0;
}

static int lsocket_obj_listen(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer backlog = luaL_checkinteger(L, 2);

    pal_socket_err err = pal_socket_listen(obj->socket, backlog);
    if (err != PAL_SOCKET_ERR_OK) {
        luaL_error(L, pal_socket_get_error_str(err));
    }

    return 0;
}

static void lsocket_accepted_cb(pal_socket_obj *o, pal_socket_err err, pal_socket_obj *new_o,
    const char *addr, uint16_t port, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);  // -4
    lua_pushlightuserdata(co, new_o);  // -3
    lua_pushstring(co, addr);  // -2
    lua_pushinteger(co, port);  // -1
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
        lc_freethread(co);
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshaccept(lua_State *L, int status, lua_KContext extra) {
    // lua_stack: [-1] = port, [-2] = addr, [-3] = new_o, [-4] = err
    pal_socket_err err = lua_tointeger(L, -4);
    pal_socket_obj *new_o = lua_touserdata(L, -3);

    switch (err) {
    case PAL_SOCKET_ERR_OK: {
        lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
        luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);
        obj->socket = new_o;
        lua_insert(L, -3);  // lua_stack: [-1] = port, [-2] = addr, [-3] = obj
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
        lua_yieldk(L, 0, 0, finshaccept);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static void lsocket_connected_cb(pal_socket_obj *o, pal_socket_err err, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
        lc_freethread(co);
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshconnect(lua_State *L, int status, lua_KContext extra) {
    // lua_stack: [-1] = err
    pal_socket_err err = luaL_checkinteger(L, -1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        break;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshconnect);
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
    return finshconnect(L, 0, 0);
}

static void lsocket_sent_cb(pal_socket_obj *o, pal_socket_err err, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
        lc_freethread(co);
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshsend(lua_State *L, int status, lua_KContext extra) {
    // lua_stack: [-1] = err
    pal_socket_err err = luaL_checkinteger(L, -1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        break;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshsend);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_send(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    lua_pushinteger(L, pal_socket_send(obj->socket, data, len, lsocket_sent_cb, L));
    return finshsend(L, 0, 0);
}

static int lsocket_obj_sendto(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    const char *addr = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);
    luaL_argcheck(L, (port >= 0) && (port <= 65535), 4, "port out of range");

    lua_pushinteger(L, pal_socket_sendto(obj->socket, data, len, addr, port, lsocket_sent_cb, L));
    return finshsend(L, 0, 0);
}

static void lsocket_recved_cb(pal_socket_obj *o, pal_socket_err err,
    const char *addr, uint16_t port, void *data, size_t len, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, port);  // -4
    lua_pushstring(co, addr);  // -3
    lua_pushlstring(co, data, len);  // -2
    lua_pushinteger(co, err);  // -1
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK) {
        lc_freethread(co);
    } else if (status != LUA_YIELD) {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
        lc_freethread(co);
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshrecv(lua_State *L, int status, lua_KContext extra) {
    // lua_stack: [-1] = err, [-2] = data, [-3] = addr, [-4] = port
    pal_socket_err err = luaL_checkinteger(L, -1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        lua_pop(L, 1);
        return 1;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshrecv);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_recv(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer maxlen = luaL_checkinteger(L, 2);
    lua_pushinteger(L, pal_socket_recv(obj->socket, maxlen, lsocket_recved_cb, L));
    return finshrecv(L, 0, 0);
}

static int finshrecvfrom(lua_State *L, int status, lua_KContext extra) {
    // lua_stack: [-1] = err, [-2] = data, [-3] = addr, [-4] = port
    pal_socket_err err = luaL_checkinteger(L, -1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        lua_pop(L, 1);  // lua_stack: [-1] = data, [-2] = addr, [-3] = port
        lua_insert(L, -3);  // lua_stack: [-1] = addr, [-2] = port, [-3] = data
        lua_insert(L, -2);  // lua_stack: [-1] = port, [-2] = addr, [-3] = data
        return 3;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshrecv);
        break;
    default:
        luaL_error(L, pal_socket_get_error_str(err));
        break;
    }
    return 0;
}

static int lsocket_obj_recvfrom(lua_State *L) {
    lsocket_obj *obj = lsocket_obj_get(L, 1);
    lua_Integer maxlen = luaL_checkinteger(L, 2);
    lua_pushinteger(L, pal_socket_recv(obj->socket, maxlen, lsocket_recved_cb, L));
    return finshrecvfrom(L, 0, 0);
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
    return 0;
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
    {"sendto", lsocket_obj_sendto},
    {"recv", lsocket_obj_recv},
    {"recvfrom", lsocket_obj_recvfrom},
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
