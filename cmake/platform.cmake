# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# directory
set(PLATFORM_DIR ${TOP_DIR}/platform)
set(PLATFORM_INC_DIR ${PLATFORM_DIR}/include)
set(PLATFORM_COMMON_DIR ${PLATFORM_DIR}/common)
set(PLATFORM_COMMON_SRC_DIR ${PLATFORM_COMMON_DIR}/src)
set(PLATFORM_COMMON_POSIX_DIR ${PLATFORM_DIR}/posix)
set(PLATFORM_COMMON_POSIX_INC_DIR ${PLATFORM_COMMON_POSIX_DIR}/include)
set(PLATFORM_COMMON_POSIX_SRC_DIR ${PLATFORM_COMMON_POSIX_DIR}/src)
set(PLATFORM_MBEDTLS_DIR ${PLATFORM_DIR}/mbedtls)
set(PLATFORM_MBEDTLS_INC_DIR ${PLATFORM_DIR}/mbedtls/include)
set(PLATFORM_MBEDTLS_SRC_DIR ${PLATFORM_MBEDTLS_DIR}/src)
set(PLATFORM_OPENSSL_DIR ${PLATFORM_DIR}/openssl)
set(PLATFORM_OPENSSL_SRC_DIR ${PLATFORM_OPENSSL_DIR}/src)
set(PLATFORM_LINUX_DIR ${PLATFORM_DIR}/linux)
set(PLATFORM_LINUX_SRC_DIR ${PLATFORM_LINUX_DIR}/src)
set(PLATFORM_ESP_DIR ${PLATFORM_DIR}/esp/components/platform)
set(PLATFORM_ESP_SRC_DIR ${PLATFORM_ESP_DIR}/src)

# collect platform headers
set(PLATFORM_HEADERS
    ${PLATFORM_INC_DIR}/pal/chip.h
    ${PLATFORM_INC_DIR}/pal/memory.h
    ${PLATFORM_INC_DIR}/pal/hap.h
    ${PLATFORM_INC_DIR}/pal/crypto/cipher.h
    ${PLATFORM_INC_DIR}/pal/crypto/md.h
    ${PLATFORM_INC_DIR}/pal/crypto/ssl.h
    ${PLATFORM_INC_DIR}/pal/net/socket.h
    ${PLATFORM_INC_DIR}/pal/net/addr.h
    ${PLATFORM_INC_DIR}/pal/net/dns.h
    ${PLATFORM_INC_DIR}/pal/nvs.h
)

# collect platform Linux include directories
set(PLATFORM_LINUX_INC_DIRS
    ${PLATFORM_INC_DIR}
    ${PLATFORM_COMMON_POSIX_INC_DIR}
)

# collect platform Linux sources
set(PLATFORM_LINUX_SRCS
    ${PLATFORM_COMMON_SRC_DIR}/hap.c
    ${PLATFORM_COMMON_POSIX_SRC_DIR}/socket.c
    ${PLATFORM_OPENSSL_SRC_DIR}/cipher.c
    ${PLATFORM_OPENSSL_SRC_DIR}/md.c
    ${PLATFORM_OPENSSL_SRC_DIR}/ssl.c
    ${PLATFORM_LINUX_SRC_DIR}/chip.c
    ${PLATFORM_LINUX_SRC_DIR}/memory.c
    ${PLATFORM_LINUX_SRC_DIR}/main.c
    ${PLATFORM_LINUX_SRC_DIR}/dns.c
    ${PLATFORM_COMMON_POSIX_SRC_DIR}/nvs.c
)

# collect platform ESP include directories
set(PLATFORM_ESP_INC_DIRS
    ${PLATFORM_INC_DIR}
    ${PLATFORM_MBEDTLS_INC_DIR}
)

# collect platform ESP sources
set(PLATFORM_ESP_SRCS
    ${PLATFORM_COMMON_SRC_DIR}/hap.c
    ${PLATFORM_COMMON_POSIX_SRC_DIR}/socket.c
    ${PLATFORM_MBEDTLS_SRC_DIR}/cipher.c
    ${PLATFORM_MBEDTLS_SRC_DIR}/md.c
    ${PLATFORM_MBEDTLS_SRC_DIR}/ssl.c
    ${PLATFORM_ESP_SRC_DIR}/ssl.c
    ${PLATFORM_ESP_SRC_DIR}/chip.c
    ${PLATFORM_ESP_SRC_DIR}/memory.c
    ${PLATFORM_ESP_SRC_DIR}/dns.c
)
