# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_openssl STATIC src/cipher.c src/md.c src/ssl.c)
target_include_directories(platform_openssl PUBLIC include)
target_link_libraries(platform_openssl PRIVATE platform third_party::HomeKitAdk ssl crypto)
target_compile_options(platform_openssl PRIVATE -Wno-deprecated-declarations)
add_library(platform::openssl ALIAS platform_openssl)
