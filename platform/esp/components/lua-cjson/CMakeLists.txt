# Copyright (c) 2021-2022 KNpTrue and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/lua-cjson.cmake)

idf_component_register(
    SRCS ${LUA_CJSON_SRCS}
    PRIV_REQUIRES lua
)

# fix warning
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough)
