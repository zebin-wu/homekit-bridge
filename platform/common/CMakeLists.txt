# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_common STATIC src/err.c)
target_link_libraries(platform_common PRIVATE platform third_party::HomeKitAdk)
add_library(platform::common ALIAS platform_common)
