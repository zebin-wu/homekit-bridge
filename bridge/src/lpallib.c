// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/board.h>

#include "lpallib.h"

static int board_getManufacturer(lua_State *L) {
    lua_pushstring(L, pal_board_get_manufacturer());
    return 1;
}

static int board_getModel(lua_State *L) {
    lua_pushstring(L, pal_board_get_model());
    return 1;
}

static int board_getSerialNumber(lua_State *L) {
    lua_pushstring(L, pal_board_get_serial_number());
    return 1;
}

static int board_getFirmwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_firmware_version());
    return 1;
}

static int board_getHardwareVersion(lua_State *L) {
    lua_pushstring(L, pal_board_get_hardware_version());
    return 1;
}

static const luaL_Reg board_funcs[] = {
    {"getManufacturer", board_getManufacturer},
    {"getModel", board_getModel},
    {"getSerialNumber", board_getSerialNumber},
    {"getFirmwareVersion", board_getFirmwareVersion},
    {"getHardwareVersion", board_getHardwareVersion},
    {NULL, NULL},
};

LUAMOD_API int luaopen_pal_board(lua_State *L) {
    luaL_newlib(L, board_funcs);

    return 1;
}
