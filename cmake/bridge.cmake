# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# directory
set(BRIDGE_DIR ${TOP_DIR}/bridge)
set(BRIDGE_SRC_DIR ${BRIDGE_DIR}/src)
set(BRIDGE_INC_DIR ${BRIDGE_DIR}/include)
set(BRIDGE_SCRIPTS_DIR ${BRIDGE_DIR}/scripts)

# collect bridge sources
set(BRIDGE_SRCS
    ${BRIDGE_SRC_DIR}/app.c
    ${BRIDGE_SRC_DIR}/lloglib.c
    ${BRIDGE_SRC_DIR}/lhaplib.c
    ${BRIDGE_SRC_DIR}/lchiplib.c
    ${BRIDGE_SRC_DIR}/lcorelib.c
    ${BRIDGE_SRC_DIR}/lc.c
    ${BRIDGE_SRC_DIR}/lhashlib.c
    ${BRIDGE_SRC_DIR}/lcipherlib.c
    ${BRIDGE_SRC_DIR}/lsocketlib.c
    ${BRIDGE_SRC_DIR}/lssllib.c
    ${BRIDGE_SRC_DIR}/ldnslib.c
    ${BRIDGE_SRC_DIR}/lnvslib.c
    ${BRIDGE_SRC_DIR}/lstreamlib.c
    ${BRIDGE_SRC_DIR}/lbase64lib.c
    ${BRIDGE_SRC_DIR}/larc4lib.c
    ${BRIDGE_SRC_DIR}/embedfs.c
)

# collect bridge headers
set(BRIDGE_HEADERS
    ${BRIDGE_INC_DIR}/app.h
    ${BRIDGE_INC_DIR}/embedfs.h
    ${BRIDGE_SRC_DIR}/app_int.h
    ${BRIDGE_SRC_DIR}/lc.h
)
