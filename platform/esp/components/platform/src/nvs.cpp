// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/nvs.h>
#include <pal/memory.h>
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

struct pal_nvs_handle {
    pal_nvs_mode mode;
    nvs::NVSHandle *handle;
};

extern "C" pal_nvs_handle *pal_nvs_open(const char *name, pal_nvs_mode mode) {
    HAPPrecondition(name);
    HAPPrecondition(mode == PAL_NVS_MODE_READONLY || mode == PAL_NVS_MODE_READWRITE);

    pal_nvs_handle *handle = static_cast<pal_nvs_handle *>(pal_mem_alloc(sizeof(*handle)));
    if (!handle) {
        HAPLogDebug(&logObject, "Failed to alloc memory.");
        return NULL;
    }
    handle->mode = mode;

    esp_err_t err;
    handle->handle = nvs::open_nvs_handle(name, NVS_READWRITE, &err).release();
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::open_nvs_handle() returned %s", esp_err_to_name(err));
        delete handle->handle;
        pal_mem_free(handle);
        return NULL;
    }

    return handle;
}

extern "C" bool pal_nvs_get(pal_nvs_handle *handle, const char *key, void *buf, size_t len) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    HAPPrecondition(buf);
    HAPPrecondition(len);

    esp_err_t err = handle->handle->get_blob(key, buf, len);
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::get_blob() returned %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

extern "C" size_t pal_nvs_get_len(pal_nvs_handle *handle, const char *key) {
    HAPPrecondition(handle);
    HAPPrecondition(key);

    size_t len;
    esp_err_t err = handle->handle->get_item_size(nvs::ItemType::BLOB, key, len);
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::get_item_size() returned %s", esp_err_to_name(err));
        return 0;
    }
    return err == ESP_OK ? len : 0;
}

extern "C" bool pal_nvs_set(pal_nvs_handle *handle, const char *key, const void *value, size_t len) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    HAPPrecondition(value);
    HAPPrecondition(len);

    if (handle->mode == PAL_NVS_MODE_READONLY) {
        NVS_LOG_ERR("No permission to set.");
        return false;
    }

    esp_err_t err = handle->handle->set_blob(key, value, len);
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::set_blob() returned %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

extern "C" bool pal_nvs_remove(pal_nvs_handle * handle, const char *key) {
    HAPPrecondition(handle);
    HAPPrecondition(key);

    if (handle->mode == PAL_NVS_MODE_READONLY) {
        NVS_LOG_ERR("No permission to set.");
        return false;
    }

    esp_err_t err = handle->handle->erase_item(key);
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::erase_item() returned %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

extern "C" bool pal_nvs_erase(pal_nvs_handle *handle) {
    HAPPrecondition(handle);

    if (handle->mode == PAL_NVS_MODE_READONLY) {
        NVS_LOG_ERR("No permission to set.");
        return false;
    }

    esp_err_t err = handle->handle->erase_all();
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::erase_all() returned %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

extern "C" bool pal_nvs_commit(pal_nvs_handle *handle) {
    HAPPrecondition(handle);

    if (handle->mode == PAL_NVS_MODE_READONLY) {
        NVS_LOG_ERR("No permission to set.");
        return false;
    }

    esp_err_t err = handle->handle->commit();
    if (err != ESP_OK) {
        HAPLogDebug(&logObject, "nvs::commit() returned %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

extern "C" void pal_nvs_close(pal_nvs_handle *handle) {
    if (handle) {
        if (handle->mode == PAL_NVS_MODE_READWRITE) {
            handle->handle->commit();
        }
        delete handle->handle;
        pal_mem_free(handle);
    }
}
