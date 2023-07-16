# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform INTERFACE)
target_include_directories(platform INTERFACE include)
target_check_cstyle(platform )
target_link_libraries(platform INTERFACE platform::${PLATFORM})

add_subdirectory(common)
add_subdirectory(${PLATFORM})

if(CONFIG_POSIX)
    add_subdirectory(posix)
endif()

if(CONFIG_MBEDTLS)
    add_subdirectory(mbedtls)
endif()

if(CONFIG_OPENSSL)
    add_subdirectory(openssl)
endif()
