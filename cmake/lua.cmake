# directory
set(LUA_DIR "${TOP_DIR}/ext/lua")

# lua
set(LUA_SRCS    "${LUA_DIR}/lapi.c"
                "${LUA_DIR}/lauxlib.c"
                "${LUA_DIR}/lbaselib.c"
                "${LUA_DIR}/lcode.c"
                "${LUA_DIR}/lcorolib.c"
                "${LUA_DIR}/lctype.c"
                "${LUA_DIR}/ldblib.c"
                "${LUA_DIR}/ldebug.c"
                "${LUA_DIR}/ldo.c"
                "${LUA_DIR}/ldump.c"
                "${LUA_DIR}/lfunc.c"
                "${LUA_DIR}/lgc.c"
                "${LUA_DIR}/linit.c"
                "${LUA_DIR}/liolib.c"
                "${LUA_DIR}/llex.c"
                "${LUA_DIR}/lmathlib.c"
                "${LUA_DIR}/lmem.c"
                "${LUA_DIR}/loadlib.c"
                "${LUA_DIR}/lobject.c"
                "${LUA_DIR}/lopcodes.c"
                "${LUA_DIR}/loslib.c"
                "${LUA_DIR}/lparser.c"
                "${LUA_DIR}/lstate.c"
                "${LUA_DIR}/lstring.c"
                "${LUA_DIR}/lstrlib.c"
                "${LUA_DIR}/ltable.c"
                "${LUA_DIR}/ltablib.c"
                "${LUA_DIR}/ltests.c"
                "${LUA_DIR}/ltm.c"
                "${LUA_DIR}/lundump.c"
                "${LUA_DIR}/lutf8lib.c"
                "${LUA_DIR}/lvm.c"
                "${LUA_DIR}/lzio.c")

set(LUA_INC_DIR "${LUA_DIR}")
