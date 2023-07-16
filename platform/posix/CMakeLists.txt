# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_posix STATIC src/net_addr.c src/nvs.c src/socket.c)
target_include_directories(platform_posix PUBLIC include)
target_link_libraries(platform_posix PRIVATE platform third_party::HomeKitAdk)
add_library(platform::posix ALIAS platform_posix)
