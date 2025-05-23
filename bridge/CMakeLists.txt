# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(BRIDGE_SRCS
    src/app.c
    src/lloglib.c
    src/lhaplib.c
    src/lchiplib.c
    src/lcorelib.c
    src/lc.c
    src/lhashlib.c
    src/lcipherlib.c
    src/lsocketlib.c
    src/ldnslib.c
    src/lnvslib.c
    src/lstreamlib.c
    src/lbase64lib.c
    src/larc4lib.c
    src/lnetiflib.c
    src/embedfs.c
)

set(BRIDGE_HEADERS
    include/app.h
    include/embedfs.h
    src/app_int.h
    src/lc.h
)

add_library(bridge STATIC ${BRIDGE_SRCS})
target_include_directories(bridge PUBLIC include)
target_link_libraries(bridge PRIVATE platform platform::common third_party::HomeKitAdk third_party::lua third_party::lua-cjson)

target_compile_definitions(bridge PUBLIC
    BRIDGE_VERSION="${PROJECT_VERSION}"
    BRIDGE_EMBEDFS_ROOT=${BRIDGE_EMBEDFS_ROOT}
)

target_add_lua_binary_embedfs(bridge
    ${BRIDGE_EMBEDFS_ROOT}
    SRC_DIRS scripts ../plugins
)
