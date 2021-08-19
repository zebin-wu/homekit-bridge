// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/board.h>

#include "app_int.h"

static int lboard_getManufacturer(lua_State *L) {
    lua_pushstring(L, pal_board_get_manufacturer());
    return 1;
}

static int lboard_getModel(lua_State *L) {
    lua_pushstring(L, pal_board_get_model());
    return 1;
}

static int lboard_getSerialNumber(lua_State *L) {
    lua_pushstring(L, pal_board_get_serial_number());
    return 1;
}

static int lboard_getFirmwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_firmware_version());
    return 1;
}

static int lboard_getHardwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_hardware_version());
    return 1;
}

static const luaL_Reg lboard_funcs[] = {
    {"getManufacturer", lboard_getManufacturer},
    {"getModel", lboard_getModel},
    {"getSerialNumber", lboard_getSerialNumber},
    {"getFirmwareVersion", lboard_getFirmwareVersion},
    {"getHardwareVersion", lboard_getHardwareVersion},
    {NULL, NULL},
};

LUAMOD_API int luaopen_board(lua_State *L) {
    luaL_newlib(L, lboard_funcs);
    return 1;
}
