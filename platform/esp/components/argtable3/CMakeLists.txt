# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/argtable3.cmake)

idf_component_register(
    INCLUDE_DIRS ${ARGTABLE3_INC_DIR}
    SRCS ${ARGTABLE3_SRCS}
)
