/**
 * Copyright (c) 2021 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include <lauxlib.h>
#include <platform/board.h>

#include "lpallib.h"

static int board_getManufacturer(lua_State *L)
{
    lua_pushstring(L, pal_board_get_manufacturer());
    return 1;
}

static int board_getModel(lua_State *L)
{
    lua_pushstring(L, pal_board_get_model());
    return 1;
}

static int board_getSerialNumber(lua_State *L)
{
    lua_pushstring(L, pal_board_get_serial_number());
    return 1;
}

static int board_getFirmwareVersion(lua_State *L)
{
    lua_pushstring(L, pal_board_get_firmware_version());
    return 1;
}

static int board_getHardwareVersion(lua_State *L)
{
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

static const luaL_Reg pallib[] = {
    {"board", NULL},
    {NULL, NULL},
};

LUAMOD_API int luaopen_pal(lua_State *L)
{
    luaL_newlib(L, pallib);

    /* set board */
    lua_newtable(L);
    luaL_setfuncs(L, board_funcs, 0);
    lua_setfield(L, -2, "board");

    return 1;
}
