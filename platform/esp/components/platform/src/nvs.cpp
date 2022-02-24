// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/nvs.h>
#include <esp_err.h>
#include <nvs_handle.hpp>
#include <HAPPlatform.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "nvs" };

#define NVS_LOG_ERR(fmt, arg...) \
    HAPLogError(&logObject, "%s: " fmt, __func__, ##arg);

static const nvs_open_mode_t open_mode_mapping[] = {
    [PAL_NVS_MODE_READONLY] = NVS_READONLY,
    [PAL_NVS_MODE_READWRITE] = NVS_READWRITE
};

extern "C" pal_nvs_handle *pal_nvs_open(const char *name, enum pal_nvs_mode mode) {
    HAPPrecondition(name);
    HAPPrecondition(mode == PAL_NVS_MODE_READONLY || mode == PAL_NVS_MODE_READWRITE);

    esp_err_t err;
    nvs::NVSHandle *handle = nvs::open_nvs_handle(name, open_mode_mapping[mode], &err).release();

    if (err != ESP_OK) {
        NVS_LOG_ERR("Failed to open NVS!");
        delete handle;
        return NULL;
    }

    return (pal_nvs_handle *)static_cast<void *>(handle);
}

extern "C" bool pal_nvs_get(pal_nvs_handle *_handle, const char *key, void *buf, size_t len) {
    HAPPrecondition(_handle);
    HAPPrecondition(key);
    HAPPrecondition(buf);
    HAPPrecondition(len);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    return handle->get_blob(key, buf, len) == ESP_OK;
}

extern "C" size_t pal_nvs_get_len(pal_nvs_handle *_handle, const char *key) {
    HAPPrecondition(_handle);
    HAPPrecondition(key);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    size_t len;
    esp_err_t err = handle->get_item_size(nvs::ItemType::BLOB_DATA, key, len);
    return err == ESP_OK ? len : 0;
}

extern "C" bool pal_nvs_set(pal_nvs_handle *_handle, const char *key, const void *value, size_t len) {
    HAPPrecondition(_handle);
    HAPPrecondition(key);
    HAPPrecondition(value);
    HAPPrecondition(len);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    return handle->set_blob(key, value, len) == ESP_OK;
}

extern "C" bool pal_nvs_remove(pal_nvs_handle *_handle, const char *key) {
    HAPPrecondition(_handle);
    HAPPrecondition(key);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    return handle->erase_item(key) == ESP_OK;
}

extern "C" bool pal_nvs_erase(pal_nvs_handle *_handle) {
    HAPPrecondition(_handle);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    return handle->erase_all() == ESP_OK;
}

extern "C" bool pal_nvs_commit(pal_nvs_handle *_handle) {
    HAPPrecondition(_handle);

    nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
    return handle->commit() == ESP_OK;
}

extern "C" void pal_nvs_close(pal_nvs_handle *_handle) {
    if (_handle) {
        nvs::NVSHandle *handle = static_cast<nvs::NVSHandle *>((void *)_handle);
        delete handle;
    }
}
