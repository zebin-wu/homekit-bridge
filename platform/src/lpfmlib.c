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
#include <lpfmlib.h>
#include <platform/board.h>

static int pfm_getManufacturer(lua_State *L)
{
    lua_pushstring(L, pfm_board_manufacturer_get());
    return 1;
}

static int pfm_getModel(lua_State *L)
{
    lua_pushstring(L, pfm_board_model_get());
    return 1;
}

static int pfm_getSerialNumber(lua_State *L)
{
    lua_pushstring(L, pfm_board_serial_number_get());
    return 1;
}

static int pfm_getFirmwareVersion(lua_State *L)
{
    lua_pushstring(L, pfm_board_firmware_version_get());
    return 1;
}

static int pfm_getHardwareVersion(lua_State *L)
{
    lua_pushstring(L, pfm_board_hardware_version_get());
    return 1;
}

static const luaL_Reg pfmlib[] = {
    {"getManufacturer", pfm_getManufacturer},
    {"getModel", pfm_getModel},
    {"getSerialNumber", pfm_getSerialNumber},
    {"getFirmwareVersion", pfm_getFirmwareVersion},
    {"getHardwareVersion", pfm_getHardwareVersion},
    {NULL, NULL},
};

LUAMOD_API int luaopen_pfm(lua_State *L)
{
    luaL_newlib(L, pfmlib);
    return 1;
}
