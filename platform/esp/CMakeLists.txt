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
compile_luac(${LUAC_BINARY} ${TOP_DIR}/platform/${HOST_PLATFORM}/luac ${LUAC_BUILD_DIR})

# generate lua binaries to work directory
set(BRIDGE_WORK_DIR ${CMAKE_BINARY_DIR}/work)
gen_lua_binary_from_dir(${BRIDGE_WORK_DIR} ${BRIDGE_LUA_DIR} ${LUAC_BINARY})

# set extra component directroies
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/system/console/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT})

# create storage.bin
spiffs_create_partition_image(storage
    ${BRIDGE_LUA_DIR}
    FLASH_IN_PROJECT
    DEPENDS lua_binary
)

if(CONFIG_HAP_MFI_HW_AUTH)
    add_definitions(-DHAVE_MFI_HW_AUTH=1)
endif()
