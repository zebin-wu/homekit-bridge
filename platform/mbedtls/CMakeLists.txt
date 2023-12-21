# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_mbedtls STATIC src/cipher.c src/md.c src/ssl.c)
target_include_directories(platform_mbedtls PUBLIC include)
target_link_libraries(platform_mbedtls PRIVATE platform third_party::HomeKitAdk mbedtls mbedcrypto)
target_compile_options(platform_mbedtls PRIVATE -Wno-deprecated-declarations)
add_library(platform::mbedtls ALIAS platform_mbedtls)
