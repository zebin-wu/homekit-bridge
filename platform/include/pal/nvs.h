// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NVS_H_
#define PLATFORM_INCLUDE_PAL_NVS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct pal_nvs_handle pal_nvs_handle;

pal_nvs_handle *pal_nvs_open(const char *namespace);

void pal_nvs_get(pal_nvs_handle *handle, const char *key, void *buf, size_t *len);

size_t pal_nvs_get_size(pal_nvs_handle *handle, const char *key);

void pal_nvs_set(pal_nvs_handle *handle, const char *key, const void *buf, size_t len);

void pal_nvs_remove(pal_nvs_handle *handle, const char *key);

void pal_nvs_earse(pal_nvs_handle *handle);

void pal_nvs_commit(pal_nvs_handle *handle);

void pal_nvs_close(pal_nvs_handle *handle);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NVS_H_
