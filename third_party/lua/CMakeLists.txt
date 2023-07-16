# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# directory
set(LUA_DIR lua)
set(LUA_INC_DIR ${LUA_DIR}/src)
set(LUA_SRC_DIR ${LUA_DIR}/src)

# collect lua sources
set(LUA_SRCS
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/linit.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
)

set(LUAC_SRCS ${LUA_SRC_DIR}/luac.c)

# collect lua headers
set(LUA_HEADERS
    ${LUA_SRC_DIR}/lapi.h
    ${LUA_SRC_DIR}/lauxlib.h
    ${LUA_SRC_DIR}/lcode.h
    ${LUA_SRC_DIR}/lctype.h
    ${LUA_SRC_DIR}/ldebug.h
    ${LUA_SRC_DIR}/ldo.h
    ${LUA_SRC_DIR}/lfunc.h
    ${LUA_SRC_DIR}/lgc.h
    ${LUA_SRC_DIR}/ljumptab.h
    ${LUA_SRC_DIR}/llex.h
    ${LUA_SRC_DIR}/llimits.h
    ${LUA_SRC_DIR}/lmem.h
    ${LUA_SRC_DIR}/lobject.h
    ${LUA_SRC_DIR}/lopcodes.h
    ${LUA_SRC_DIR}/lopnames.h
    ${LUA_SRC_DIR}/lparser.h
    ${LUA_SRC_DIR}/lprefix.h
    ${LUA_SRC_DIR}/lstate.h
    ${LUA_SRC_DIR}/lstring.h
    ${LUA_SRC_DIR}/ltable.h
    ${LUA_SRC_DIR}/ltm.h
    ${LUA_SRC_DIR}/lua.h
    ${LUA_SRC_DIR}/luaconf.h
    ${LUA_SRC_DIR}/lualib.h
    ${LUA_SRC_DIR}/lundump.h
    ${LUA_SRC_DIR}/lvm.h
    ${LUA_SRC_DIR}/lzio.h
)

add_library(lua STATIC ${LUA_SRCS})
target_include_directories(lua PUBLIC ${LUA_INC_DIR})
target_link_libraries(lua PRIVATE m)
add_library(third_party::lua ALIAS lua)

if(${PLATFORM} STREQUAL linux)
    target_compile_definitions(lua PUBLIC
        LUA_USE_LINUX
    )
endif()

# compile host luac
find_program(MAKE NAMES "make")
if(NOT MAKE)
    message(FATAL_ERROR "make not found")    
endif()
set(HOST_LUAC_BINARY ${CMAKE_CURRENT_BINARY_DIR}/host/luac)
add_custom_command(OUTPUT ${HOST_LUAC_BINARY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${LUA_DIR}
    COMMAND ${MAKE} -s 1> /dev/null
    COMMAND cp src/luac ${HOST_LUAC_BINARY}
    COMMAND ${MAKE} -s clean
    DEPENDS ${LUA_SRCS} ${LUAC_SRCS} ${LUA_HEADERS}
    COMMENT "Compiling luac"
)
add_custom_target(HOST_LUAC_BINARY DEPENDS ${HOST_LUAC_BINARY})
add_executable(host_luac IMPORTED GLOBAL)
add_dependencies(host_luac HOST_LUAC_BINARY)
set_target_properties(host_luac PROPERTIES IMPORTED_LOCATION ${HOST_LUAC_BINARY})
