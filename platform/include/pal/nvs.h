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

/* The maximum length of the NVS key, not include '\0' */
#define PAL_NVS_KEY_MAX_LEN 15

/**
 * Opaque structure for the NVS handle.
 */
typedef struct pal_nvs_handle pal_nvs_handle;

/**
 * Open a NVS(non-volatile storage) handle with a given namespace.
 *
 * @param name Name of the namespace. Maximum length is PAL_NVS_KEY_MAX_LEN. Shouldn't be empty.
 * @returns a NVS handle or NULL if the open fails.
 */
pal_nvs_handle *pal_nvs_open(const char *name);

/**
 * Get value for given key.
 *
 * @param handle The NVS handle.
 * @param key Key name. Maximum length is PAL_NVS_KEY_MAX_LEN. Shouldn't be empty.
 * @param buf A buffer to hold the value.
 * @param len Length of @p buf. Use \c pal_nvs_get_len to query the length beforehand.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_nvs_get(pal_nvs_handle *handle, const char *key, void *buf, size_t len);

/**
 * Get the value length in bytes.
 *
 * @param handle The NVS handle.
 * @param key Key name. Maximum length is PAL_NVS_KEY_MAX_LEN. Shouldn't be empty.
 * @returns the length of the value.
 */
size_t pal_nvs_get_len(pal_nvs_handle *handle, const char *key);

/**
 * Set the value for given key.
 *
 * @param handle The NVS handle.
 * @param key Key name. Maximum length is PAL_NVS_KEY_MAX_LEN. Shouldn't be empty.
 * @param value The value to set.
 * @param len Length of value.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_nvs_set(pal_nvs_handle *handle, const char *key, const void *value, size_t len);

/**
 * Remove the key-value pair for given key.
 * 
 * @param handle The NVS handle.
 * @param key Key name. Maximum length is PAL_NVS_KEY_MAX_LEN. Shouldn't be empty.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_nvs_remove(pal_nvs_handle *handle, const char *key);

/**
 * Erase all key-value pairs.
 *
 * @param handle The NVS handle.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_nvs_erase(pal_nvs_handle *handle);

/**
 * Write any pending changes to non-volatile storage.
 *
 * @param handle The NVS handle.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_nvs_commit(pal_nvs_handle *handle);

/**
 * Close the handle and free any allocated resources.
 *
 * @param handle The NVS handle.
 *
 * @return true on success.
 * @return false on failure.
 */
void pal_nvs_close(pal_nvs_handle *handle);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NVS_H_
