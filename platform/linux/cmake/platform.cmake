# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# system api
set(CONFIG_POSIX ON)

# crypto library
set(CONFIG_OPENSSL ON)
set(CONFIG_MBEDTLS OFF)

# set the work directory
set(BRIDGE_WORK_DIR "/usr/local/lib/${CMAKE_PROJECT_NAME}")

# set the embedfs root
set(BRIDGE_EMBEDFS_ROOT bridge_embedfs_root)

add_compile_options(-Wall -Werror)

# install binaries
install(TARGETS ${CMAKE_PROJECT_NAME}
    DESTINATION bin
)
