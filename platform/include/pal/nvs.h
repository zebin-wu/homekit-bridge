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
#include <stdbool.h>

#define PAL_NVS_KEY_MAX_LEN 31

typedef enum pal_nvs_mode {
    PAL_NVS_MODE_READONLY,
    PAL_NVS_MODE_READWRITE,
} pal_nvs_mode;

typedef struct pal_nvs_handle pal_nvs_handle;

pal_nvs_handle *pal_nvs_open(const char *name, pal_nvs_mode mode);

bool pal_nvs_get(pal_nvs_handle *handle, const char *key, void *buf, size_t len);

size_t pal_nvs_get_len(pal_nvs_handle *handle, const char *key);

bool pal_nvs_set(pal_nvs_handle *handle, const char *key, const void *value, size_t len);

bool pal_nvs_remove(pal_nvs_handle *handle, const char *key);

bool pal_nvs_erase(pal_nvs_handle *handle);

bool pal_nvs_commit(pal_nvs_handle *handle);

void pal_nvs_close(pal_nvs_handle *handle);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NVS_H_
