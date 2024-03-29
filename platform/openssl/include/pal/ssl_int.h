// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_OPENSSL_INCLUDE_PAL_CRYPTO_SSL_INT_H_
#define PLATFORM_OPENSSL_INCLUDE_PAL_CRYPTO_SSL_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SSL module.
 */
void pal_ssl_init();

/**
 * De-initialize SSL module.
 */
void pal_ssl_deinit();

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_OPENSSL_INCLUDE_PAL_CRYPTO_SSL_INT_H_
