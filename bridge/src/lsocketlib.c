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

static bool lsocket_port_valid(lua_Integer port) {
    return ((port >= 0) && (port <= 65535));
}

static int lsocket_create(lua_State *L) {
    pal_socket_type type = luaL_checkoption(L, 1, NULL, (const char *[]) {
        "TCP",
        "UDP"
    });
    pal_socket_domain domain = luaL_checkoption(L, 2, NULL, (const char *[]) {
        "IPV4",
        "IPV6"
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
    pal_socket_bind(obj->socket, addr, port);
    return 1;
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
