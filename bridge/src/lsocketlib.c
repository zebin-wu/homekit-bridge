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

static bool lsocket_port_valid(lua_Integer port) {
    return ((port >= 0) && (port <= 65535));
}

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

static int lsocket_obj_bind(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lsocket_port_valid(port)) {
        luaL_argerror(L, 3, "port out of range");
    }
    pal_socket_err err = pal_socket_bind(obj->socket, addr, port);
    if (err != PAL_SOCKET_ERR_OK) {
        luaL_error(L, "failed to bind");
    }
    return 0;
}

static int lsocket_obj_listen(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    lua_Integer backlog = luaL_checkinteger(L, 2);

    pal_socket_err err = pal_socket_listen(obj->socket, backlog);
    if (err != PAL_SOCKET_ERR_OK) {
        luaL_error(L, "failed to listen");
    }

    return 0;
}

static void lsocket_accepted_cb(pal_socket_obj *o, pal_socket_err err, pal_socket_obj *new_o, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    lua_pushlightuserdata(co, new_o);
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK || status == LUA_YIELD) {
        if (status == LUA_OK) {
            lc_freethread(co);
        }
    } else {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshaccept(lua_State *L, int status, lua_KContext extra) {
    pal_socket_obj *new_o = lua_touserdata(L, -1);
    pal_socket_err err = lua_tointeger(L, -2);
    lua_pop(L, 2);

    switch (err) {
    case PAL_SOCKET_ERR_OK: {
        lsocket_obj *obj = lua_newuserdata(L, sizeof(lsocket_obj));
        luaL_setmetatable(L, LUA_SOCKET_OBJECT_NAME);
        obj->socket = new_o;
        return 1;
    }
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshaccept);
        break;
    default:
        luaL_error(L, "failed to accept");
        break;
    }
    return 0;
}

static int lsocket_obj_accept(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    pal_socket_obj *new_o = NULL;

    lua_pushinteger(L,
        pal_socket_accept(obj->socket, &new_o, lsocket_accepted_cb, L));
    lua_pushlightuserdata(L, new_o);
    return finshaccept(L, 0, 0);
}

static void lsocket_connected_cb(pal_socket_obj *o, pal_socket_err err, void *arg) {
    lua_State *L = app_get_lua_main_thread();
    lua_State *co = arg;
    int status, nres;

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    status = lua_resume(co, L, 0, &nres);
    if (status == LUA_OK || status == LUA_YIELD) {
        if (status == LUA_OK) {
            lc_freethread(co);
        }
    } else {
        luaL_traceback(L, co, lua_tostring(co, -1), 1);
        HAPLogError(&lsocket_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshconnect(lua_State *L, int status, lua_KContext extra) {
    pal_socket_err err = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    switch (err) {
    case PAL_SOCKET_ERR_OK:
        break;
    case PAL_SOCKET_ERR_IN_PROGRESS:
        lua_yieldk(L, 0, 0, finshconnect);
        break;
    default:
        luaL_error(L, "failed to connect");
        break;
    }
    return 0;
}

static int lsocket_obj_connect(lua_State *L) {
    lsocket_obj *obj = luaL_checkudata(L, 1, LUA_SOCKET_OBJECT_NAME);
    const char *addr = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    if (!lsocket_port_valid(port)) {
        luaL_argerror(L, 3, "port out of range");
    }
    lua_pushinteger(L, pal_socket_connect(obj->socket, addr,
        port, lsocket_connected_cb, L));
    return finshconnect(L, 0, 0);
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
    {"bind", lsocket_obj_bind},
    {"listen", lsocket_obj_listen},
    {"accept", lsocket_obj_accept},
    {"connect", lsocket_obj_connect},
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
