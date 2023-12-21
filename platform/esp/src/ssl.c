// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <esp_err.h>
#include <esp_crt_bundle.h>
#include <pal/ssl_int.h>
#include <HAPPlatform.h>

void pal_ssl_init() {
}

void pal_ssl_deinit() {
}

void pal_ssl_set_default_ca_chain(mbedtls_ssl_config *conf) {
    HAPAssert(esp_crt_bundle_attach(conf) == ESP_OK);
}
