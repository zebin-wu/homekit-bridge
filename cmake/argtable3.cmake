# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# directory
set(ARGTABLE3_DIR ${TOP_DIR}/ext/argtable3)
set(ARGTABLE3_INC_DIR ${ARGTABLE3_DIR}/src)
set(ARGTABLE3_SRC_DIR ${ARGTABLE3_DIR}/src)

# collect argtable3 sources
set(ARGTABLE3_SRCS
    ${ARGTABLE3_SRC_DIR}/arg_cmd.c
    ${ARGTABLE3_SRC_DIR}/arg_date.c
    ${ARGTABLE3_SRC_DIR}/arg_dbl.c
    ${ARGTABLE3_SRC_DIR}/arg_dstr.c
    ${ARGTABLE3_SRC_DIR}/arg_end.c
    ${ARGTABLE3_SRC_DIR}/arg_file.c
    ${ARGTABLE3_SRC_DIR}/arg_hashtable.c
    ${ARGTABLE3_SRC_DIR}/arg_int.c
    ${ARGTABLE3_SRC_DIR}/arg_lit.c
    ${ARGTABLE3_SRC_DIR}/arg_rem.c
    ${ARGTABLE3_SRC_DIR}/arg_rex.c
    ${ARGTABLE3_SRC_DIR}/arg_str.c
    ${ARGTABLE3_SRC_DIR}/arg_utils.c
    ${ARGTABLE3_SRC_DIR}/argtable3.c
)
