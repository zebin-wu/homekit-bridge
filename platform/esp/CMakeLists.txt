# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${TOP_DIR}/cmake/project.cmake)
include(${TOP_DIR}/cmake/bridge.cmake)
include(${TOP_DIR}/cmake/extension.cmake)

# get host platform
get_host_platform(HOST_PLATFORM)

# compile luac
set(LUAC_BUILD_DIR ${CMAKE_BINARY_DIR}/luac)
set(LUAC_BINARY ${LUAC_BUILD_DIR}/luac)
compile_luac(${LUAC_BINARY}
    ${TOP_DIR}/platform/${HOST_PLATFORM}/luac
    ${LUAC_BUILD_DIR}
    DEPENDS ${LUA_SRCS} ${LUAC_SRCS} ${LUA_HEADERS}
)

# set extra component directroies
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/system/console/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT})

set(SCRIPTS_DST_DIR ${CMAKE_BINARY_DIR}/bridge_scripts)

if(CONFIG_LUA_APP_DEFAULT)
set(SCRIPTS_SRC_DIRS ${BRIDGE_SCRIPTS_DIR})
configure_file(${TOP_DIR}/config/config.lua.in ${SCRIPTS_DST_DIR}/config.lua)
endif()

if(CONFIG_LUA_APP_EXAMPLE)
set(SCRIPTS_SRC_DIRS ${BRIDGE_SCRIPTS_DIR} ${TOP_DIR}/example)
endif()

# generate lua binaries to work directory
gen_lua_binary_from_dir(bridge_scripts
    ${SCRIPTS_DST_DIR}
    ${LUAC_BINARY}
    SRC_DIRS ${SCRIPTS_SRC_DIRS}
)

# create storage.bin
spiffs_create_partition_image(storage
    ${SCRIPTS_DST_DIR}
    FLASH_IN_PROJECT
    DEPENDS bridge_scripts
)

if(CONFIG_HAP_MFI_HW_AUTH)
    add_definitions(-DHAVE_MFI_HW_AUTH=1)
endif()
