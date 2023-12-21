# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_esp STATIC
    src/app_console.c
    src/app_main.c
    src/app_spiffs.c
    src/app_wifi.c
    src/cmd_system.c
    src/chip.c
    src/dns.c
    src/hap.c
    src/net_if.c
    src/nvs.cpp
    src/ssl.c
)

target_include_directories(platform_esp PUBLIC include)

target_link_libraries(platform_esp
    bridge
    platform
    platform::mbedtls
    platform::posix
    third_party::HomeKitAdk
    idf::freertos
    idf::nvs_flash
    idf::console
    idf::mbedtls
    idf::spiffs
    idf::esp_wifi
    idf::esp_timer
    idf::nvs_flash
    idf::driver
)

add_library(platform::esp ALIAS platform_esp)
