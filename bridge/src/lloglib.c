// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <HAPLog.h>
#include <lauxlib.h>

#include "app_int.h"
#include "lloglib.h"

#define LUA_LOGGER_NAME "logger"

static const HAPLogObject llog_default_logger = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = NULL,
};

typedef struct __attribute__((__packed__)) {
    HAPLogObject obj;
    char category[1];
} llog_logger;

static int llog_get_logger(lua_State *L) {
    if (lua_isnone(L, 1)) {
        lua_pushlightuserdata(L, (void *)&llog_default_logger);
        luaL_setmetatable(L, LUA_LOGGER_NAME);
        return 1;
    }

    luaL_checktype(L, 1, LUA_TSTRING);
    size_t len;
    const char *str = lua_tolstring(L, 1, &len);
    llog_logger *logger = lua_newuserdata(L, sizeof(llog_logger) + len);
    HAPRawBufferCopyBytes(logger->category, str, len + 1);
    logger->obj.category = logger->category;
    logger->obj.subsystem = APP_BRIDGE_LOG_SUBSYSTEM;
    luaL_setmetatable(L, LUA_LOGGER_NAME);
    return 1;
}

static int llog_log_with_type(lua_State *L, HAPLogType type) {
    HAPLogObject *logger = luaL_checkudata(L, 1, LUA_LOGGER_NAME);
    HAPLogWithType(logger, type, "%s", luaL_checkstring(L, 2));
    return 0;
}

static int llog_logger_debug(lua_State *L) {
    return llog_log_with_type(L, kHAPLogType_Debug);
}

static int llog_logger_info(lua_State *L) {
    return llog_log_with_type(L, kHAPLogType_Info);
}

static int llog_logger_default(lua_State *L) {
    return llog_log_with_type(L, kHAPLogType_Default);
}

static int llog_logger_error(lua_State *L) {
    return llog_log_with_type(L, kHAPLogType_Error);
}

static int llog_logger_fault(lua_State *L) {
    return llog_log_with_type(L, kHAPLogType_Fault);
}

static int llog_logger_gc(lua_State *L) {
    return 0;
}

static int llog_logger_tostring(lua_State *L) {
    HAPLogObject *logger = luaL_checkudata(L, 1, LUA_LOGGER_NAME);
    lua_pushfstring(L, "logger (%p)", logger);
    return 1;
}

static const luaL_Reg loglib[] = {
    {"getLogger", llog_get_logger},
    {NULL, NULL},
};

/*
 * methods for logger
 */
static const luaL_Reg meth[] = {
    {"debug", llog_logger_debug},
    {"info", llog_logger_info},
    {"default", llog_logger_default},
    {"error", llog_logger_error},
    {"fault", llog_logger_fault},
    {NULL, NULL}
};

/*
 * metamethods for logger
 */
static const luaL_Reg metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", llog_logger_gc},
    {"__close", llog_logger_gc},
    {"__tostring", llog_logger_tostring},
    {NULL, NULL}
};

static void createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_LOGGER_NAME);  /* metatable for logger */
    luaL_setfuncs(L, metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);  /* create method table */
    luaL_setfuncs(L, meth, 0);  /* add logger methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_log(lua_State *L) {
    luaL_newlib(L, loglib); /* new module */
    createmeta(L);
    return 1;
}
