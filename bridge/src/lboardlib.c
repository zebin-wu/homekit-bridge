// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/board.h>

#include "app_int.h"

typedef enum {
    LBOARD_MFG,
    LBOARD_MODEL,
    LBOARD_SN,
    LBOARD_FW_VER,
    LBOARD_HW_VER,
} lboard_info_type;

static const char *(*lboard_get_info_funcs[])() = {
    [LBOARD_MFG] = pal_board_get_manufacturer,
    [LBOARD_MODEL] = pal_board_get_model,
    [LBOARD_SN] = pal_board_get_serial_number,
    [LBOARD_FW_VER] = pal_board_get_firmware_version,
    [LBOARD_HW_VER] = pal_board_get_hardware_version,
};

static int lboard_get_info(lua_State *L) {
    lua_pushstring(L, lboard_get_info_funcs[luaL_checkoption(L, 1, NULL, (const char *[]) {
        [LBOARD_MFG] = "mfg",
        [LBOARD_MODEL] = "model",
        [LBOARD_SN] = "sn",
        [LBOARD_FW_VER] = "fwver",
        [LBOARD_HW_VER] = "hwver",
    })]());
    return 1;
}

static const luaL_Reg lboard_funcs[] = {
    {"getInfo", lboard_get_info},
    {NULL, NULL},
};

LUAMOD_API int luaopen_board(lua_State *L) {
    luaL_newlib(L, lboard_funcs);
    return 1;
}
