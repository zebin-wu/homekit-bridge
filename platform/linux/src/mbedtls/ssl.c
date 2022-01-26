// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdbool.h>
#include <mbedtls/x509.h>
#include <pal/crypto/ssl_int.h>
#include <HAPPlatform.h>

static mbedtls_x509_crt default_ca_chain;
static bool isinited;

void pal_ssl_init() {
    HAPPrecondition(!isinited);
    mbedtls_x509_crt_init(&default_ca_chain);
    mbedtls_x509_crt_parse_path(&default_ca_chain, "/etc/ssl/certs");
    isinited = true;
}

void pal_ssl_deinit() {
    HAPPrecondition(isinited);
    mbedtls_x509_crt_free(&default_ca_chain);
    isinited = false;
}

void pal_ssl_set_default_ca_chain(mbedtls_ssl_config *conf) {
    HAPPrecondition(isinited);
    mbedtls_ssl_conf_ca_chain(conf, &default_ca_chain, NULL);
}
