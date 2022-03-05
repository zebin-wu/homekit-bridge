// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pal/memory.h>
#include <pal/nvs.h>

#include <HAPPlatform.h>
#include <HAPPlatformFileManager.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "nvs" };

#define NVS_LOG_ERR(fmt, arg...) \
    HAPLogError(&logObject, "%s: " fmt, __func__, ##arg);

#define PAL_NVS_MAGIC "nvs"
#define PAL_NVS_MAGIC_LEN sizeof(PAL_NVS_MAGIC) - 1

struct pal_nvs_item {
    char key[PAL_NVS_KEY_MAX_LEN + 1];
    size_t len;
    SLIST_ENTRY(pal_nvs_item) list_entry;
    char value[0];
};

struct pal_nvs_handle {
    uint32_t using_count;
    bool changed;
    char *name;
    SLIST_HEAD(pal_nvs_item_list_head, pal_nvs_item) item_list_head;
    LIST_ENTRY(pal_nvs_handle) list_entry;
};

static bool ginited;
static char *gnvs_dir;
static LIST_HEAD(pal_nvs_handle_list_head, pal_nvs_handle) ghandle_list_head;

static ssize_t read_all(int fd, void *buf, size_t len) {
    ssize_t rc;
    size_t readbytes = 0;

    while (readbytes < len) {
        do {
            rc = read(fd, buf + readbytes, len - readbytes);
        } while (rc == -1 && errno == EINTR);
        if (rc < 0) {
            return rc;
        } else if (rc == 0) {
            break;
        }
        readbytes += rc;
    }
    return readbytes;
}

void pal_nvs_init(const char *dir) {
    HAPPrecondition(ginited == false);
    size_t len = strlen(dir);
    gnvs_dir = pal_mem_alloc(len + 1);
    HAPAssert(gnvs_dir);
    memcpy(gnvs_dir, dir, len);
    gnvs_dir[len] = '\0';
    LIST_INIT(&ghandle_list_head);
    ginited = true;
}

void pal_nvs_deinit() {
    HAPPrecondition(ginited == true);
    for (struct pal_nvs_handle *t = LIST_FIRST(&ghandle_list_head); t;) {
        struct pal_nvs_handle *cur = t;
        t = LIST_NEXT(t, list_entry);
        pal_nvs_close(cur);
    }
    LIST_INIT(&ghandle_list_head);
    pal_mem_free(gnvs_dir);
    ginited = false;
}

static void pal_nvs_remove_all_items(pal_nvs_handle *handle) {
    for (struct pal_nvs_item *t = SLIST_FIRST(&handle->item_list_head); t;) {
        struct pal_nvs_item *cur = t;
        t = SLIST_NEXT(t, list_entry);
        pal_mem_free(cur);
    }
    SLIST_INIT(&handle->item_list_head);
}

pal_nvs_handle *pal_nvs_open(const char *name) {
    HAPPrecondition(ginited);
    HAPPrecondition(name);
    size_t name_len = strlen(name);
    HAPPrecondition(name_len > 0 && name_len <= PAL_NVS_KEY_MAX_LEN);

    pal_nvs_handle *handle;
    LIST_FOREACH(handle, &ghandle_list_head, list_entry) {
        if (!strcmp(handle->name, name)) {
            handle->using_count++;
            return handle;
        }
    }

    handle = pal_mem_alloc(sizeof(*handle));
    if (!handle) {
        NVS_LOG_ERR("Failed to alloc memory.");
        return NULL;
    }

    handle->name = pal_mem_alloc(name_len + 1);
    if (!handle->name) {
        NVS_LOG_ERR("Failed to alloc memory.");
        goto err;
    }
    memcpy(handle->name, name, name_len);
    handle->name[name_len] = '\0';

    handle->using_count = 1;
    handle->changed = false;
    SLIST_INIT(&handle->item_list_head);

    char path[256];
    int len = snprintf(path, sizeof(path), "%s/%s", gnvs_dir, name);
    if (len < 0 || path[len - 1] != name[name_len - 1]) {
        NVS_LOG_ERR("Namespace '%s' too long.", name);
        goto err1;
    }

    int fd;
    do {
        fd = open(path, O_RDONLY);
    } while (fd == -1 && errno == EINTR);

    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            goto done;
        }
        HAPAssert(fd == -1);
        NVS_LOG_ERR("open %s failed: %d.", path, _errno);
        goto err1;
    }

    ssize_t rc;
    char magic[PAL_NVS_MAGIC_LEN];

    rc = read_all(fd, magic, sizeof(magic));
    if (rc < 0) {
        int _errno = errno;
        HAPAssert(rc == -1);
        NVS_LOG_ERR("read %s failed: %d.", path, _errno);
        goto err2;
    }

    if (rc != sizeof(magic) || memcmp(magic, PAL_NVS_MAGIC, sizeof(magic))) {
        NVS_LOG_ERR("Invalid data format.");
        goto err2;
    }

    while (1) {
        size_t len;
        rc = read_all(fd, &len, sizeof(len));
        if (rc < 0) {
            int _errno = errno;
            HAPAssert(rc == -1);
            NVS_LOG_ERR("read %s failed: %d.", path, _errno);
            goto err3;
        } else if (rc == 0) {
            break;
        }
        if (rc != sizeof(len) || len == 0 || len > PAL_NVS_KEY_MAX_LEN) {
            NVS_LOG_ERR("Invalid data format.");
            goto err3;
        }

        char key[len + 1];
        rc = read_all(fd, &key, len);
        if (rc <= 0) {
            int _errno = errno;
            HAPAssert(rc == -1);
            NVS_LOG_ERR("read %s failed: %d.", path, _errno);
            goto err3;
        }
        if (rc != len) {
            NVS_LOG_ERR("Invalid data format.");
            goto err3;
        }
        key[len] = '\0';

        rc = read_all(fd, &len, sizeof(len));
        if (rc < 0) {
            int _errno = errno;
            HAPAssert(rc == -1);
            NVS_LOG_ERR("read %s failed: %d.", path, _errno);
            goto err3;
        } else if (rc == 0) {
            NVS_LOG_ERR("Invalid data format.");
            goto err3;
        }

        if (len == 0) {
            NVS_LOG_ERR("Invalid data format.");
            goto err3;
        }

        struct pal_nvs_item *item = pal_mem_alloc(sizeof(*item) + len);
        if (!item) {
            NVS_LOG_ERR("Failed to alloc memory.");
            goto err3;
        }

        rc = read_all(fd, item->value, len);
        if (rc <= 0) {
            int _errno = errno;
            HAPAssert(rc == -1);
            NVS_LOG_ERR("read %s failed: %d.", path, _errno);
            pal_mem_free(item);
            goto err3;
        }
        if (rc != len) {
            NVS_LOG_ERR("Invalid data format.");
            pal_mem_free(item);
            goto err3;
        }
        item->len = len;
        SLIST_INSERT_HEAD(&handle->item_list_head, item, list_entry);
        snprintf(item->key, sizeof(item->key), "%s", key);
    }

done:
    LIST_INSERT_HEAD(&ghandle_list_head, handle, list_entry);
    return handle;

err3:
    pal_nvs_remove_all_items(handle);
err2:
    close(fd);
err1:
    pal_mem_free(handle->name);
err:
    pal_mem_free(handle);
    return NULL;
}

static struct pal_nvs_item *pal_nvs_find_key(pal_nvs_handle *handle, const char *key) {
    struct pal_nvs_item *t;
    SLIST_FOREACH(t, &handle->item_list_head, list_entry) {
        if (!strcmp(t->key, key)) {
            return t;
        }
    }
    return NULL;
}

bool pal_nvs_get(pal_nvs_handle *handle, const char *key, void *buf, size_t len) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    size_t key_len = strlen(key);
    HAPPrecondition(key_len > 0 && key_len <= PAL_NVS_KEY_MAX_LEN);
    HAPPrecondition(buf);
    HAPPrecondition(len);

    struct pal_nvs_item *item = pal_nvs_find_key(handle, key);
    if (item) {
        HAPAssert(len == item->len);
        memcpy(buf, item->value, len);
        return true;
    }

    HAPLog(&logObject, "No key '%s' in name '%s'.", key, handle->name);
    return false;
}

size_t pal_nvs_get_len(pal_nvs_handle *handle, const char *key) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    size_t key_len = strlen(key);
    HAPPrecondition(key_len > 0 && key_len <= PAL_NVS_KEY_MAX_LEN);

    struct pal_nvs_item *item = pal_nvs_find_key(handle, key);
    if (item) {
        return item->len;
    }
    return 0;
}

bool pal_nvs_set(pal_nvs_handle *handle, const char *key, const void *value, size_t len) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    size_t key_len = strlen(key);
    HAPPrecondition(key_len > 0 && key_len <= PAL_NVS_KEY_MAX_LEN);
    HAPPrecondition(value);
    HAPPrecondition(len);

    size_t keylen = strlen(key);
    HAPPrecondition(keylen <= PAL_NVS_KEY_MAX_LEN);

    for (struct pal_nvs_item **t = &SLIST_FIRST(&handle->item_list_head); *t;
        t = &SLIST_NEXT(*t, list_entry)) {
        if (!strcmp((*t)->key, key)) {
            if ((*t)->len != len) {
                struct pal_nvs_item *item = pal_mem_realloc(*t, sizeof(**t) + len);
                if (!item) {
                    NVS_LOG_ERR("Failed to alloc memory.");
                    return false;
                }
                *t = item;
                (*t)->len = len;
            } else if (!memcmp((*t)->value, value, len)) {
                return true;
            }
            memcpy((*t)->value, value, len);
            handle->changed = true;
            return true;
        }
    }

    struct pal_nvs_item *item = pal_mem_alloc(sizeof(*item) + len);
    if (!item) {
        NVS_LOG_ERR("Failed to alloc memory.");
        return false;
    }
    item->len = len;
    SLIST_INSERT_HEAD(&handle->item_list_head, item, list_entry);
    memcpy(item->key, key, keylen);
    item->key[keylen] = '\0';
    memcpy(item->value, value, len);
    handle->changed = true;
    return true;
}

bool pal_nvs_remove(pal_nvs_handle *handle, const char *key) {
    HAPPrecondition(handle);
    HAPPrecondition(key);
    size_t key_len = strlen(key);
    HAPPrecondition(key_len > 0 && key_len <= PAL_NVS_KEY_MAX_LEN);

    for (struct pal_nvs_item **t = &SLIST_FIRST(&handle->item_list_head); *t;
        t = &SLIST_NEXT(*t, list_entry)) {
        if (!strcmp((*t)->key, key)) {
            struct pal_nvs_item *cur = *t;
            *t = SLIST_NEXT(cur, list_entry);
            pal_mem_free(cur);
            handle->changed = true;
            return true;
        }
    }
    return false;
}

bool pal_nvs_erase(pal_nvs_handle *handle) {
    HAPPrecondition(handle);

    if (SLIST_FIRST(&handle->item_list_head)) {
        handle->changed = true;
    }
    pal_nvs_remove_all_items(handle);
    return true;
}

static size_t write_all(int fd, const void *buf, size_t len) {
    ssize_t rc;
    size_t written = 0;

    while (written < len) {
        do {
            rc = write(fd, buf + written, len - written);
        } while (rc == -1 && errno == EINTR);
        if (rc < 0) {
            return rc;
        } else if (rc == 0) {
            break;
        }
        written += rc;
    }
    return written;
}

static bool write_all_to_tmp_file(int fd, const char *path, const char *dir, const void *buf, size_t len) {
    ssize_t rc = write_all(fd, buf, len);
    if (rc < 0) {
        int _errno = errno;
        HAPAssert(rc == -1);
        NVS_LOG_ERR("write to temporary file %s failed: %d.", path, _errno);
        return false;
    }
    if (rc != len) {
        NVS_LOG_ERR("Error writing temporary file %s in %s.", path, dir);
        return false;
    }
    return true;
}

bool pal_nvs_commit(pal_nvs_handle *handle) {
    HAPPrecondition(handle);

    if (handle->changed == false) {
        return true;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", gnvs_dir, handle->name);

    if (SLIST_FIRST(&handle->item_list_head) == NULL) {
        return HAPPlatformFileManagerRemoveFile(path) == kHAPError_None;
    }

    // Create directory.
    HAPError err = HAPPlatformFileManagerCreateDirectory(gnvs_dir);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        NVS_LOG_ERR("Create directory %s failed.", gnvs_dir);
        return false;
    }

    // Open the target directory.
    DIR *dir = opendir(gnvs_dir);
    if (!dir) {
        int _errno = errno;
        NVS_LOG_ERR("opendir %s failed: %d.", gnvs_dir, _errno);
        return false;
    }
    int dir_fd = dirfd(dir);
    if (dir_fd < 0) {
        int _errno = errno;
        HAPAssert(dir_fd == -1);
        NVS_LOG_ERR("dirfd %s failed: %d.", gnvs_dir, _errno);
        goto err;
    }

    // Create the filename of the temporary file.
    char tmp_path[256];
    err = HAPStringWithFormat(tmp_path, sizeof(tmp_path), "%s-tmp", handle->name);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        NVS_LOG_ERR("Not enough resources to get path: %s-tmp", handle->name);
        goto err;
    }

    // Open the tempfile
    int tmp_fd;
    do {
        tmp_fd = openat(dir_fd, tmp_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR);
    } while (tmp_fd == -1 && errno == EINTR);
    if (tmp_fd < 0) {
        int _errno = errno;
        HAPAssert(tmp_fd == -1);
        NVS_LOG_ERR("open %s in %s failed: %d.", tmp_path, gnvs_dir, _errno);
        goto err;
    }

    // Write magic
    if (!write_all_to_tmp_file(tmp_fd, tmp_path,
        gnvs_dir, PAL_NVS_MAGIC, PAL_NVS_MAGIC_LEN)) {
        goto err1;
    }

    struct pal_nvs_item *t;
    SLIST_FOREACH(t, &handle->item_list_head, list_entry) {
        size_t len = strlen(t->key);
        if (!write_all_to_tmp_file(tmp_fd, tmp_path, gnvs_dir, &len, sizeof(len))) {
            goto err1;
        }
        if (!write_all_to_tmp_file(tmp_fd, tmp_path, gnvs_dir, t->key, len)) {
            goto err1;
        }
        if (!write_all_to_tmp_file(tmp_fd, tmp_path, gnvs_dir, &t->len, sizeof(t->len))) {
            goto err1;
        }
        if (!write_all_to_tmp_file(tmp_fd, tmp_path, gnvs_dir, t->value, t->len)) {
            goto err1;
        }
    }

    // Try to synchronize and close the temporary file.
    {
        int e;
        do {
            e = fsync(tmp_fd);
        } while (e == -1 && errno == EINTR);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            NVS_LOG_ERR("fsync of temporary file %s failed: %d.", tmp_path, _errno);
        }
        (void) close(tmp_fd);
    }

    // Fsync dir
    {
        int e;
        do {
            e = fsync(dir_fd);
        } while (e == -1 && errno == EINTR);
        if (e < 0) {
            int _errno = errno;
            HAPAssert(e == -1);
            NVS_LOG_ERR("fsync of the directory %s failed: %d", gnvs_dir, _errno);
            goto err;
        }
    }

    // Rename file
    {
        int e = renameat(dir_fd, tmp_path, dir_fd, handle->name);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            NVS_LOG_ERR("rename of temporary file %s to %s failed: %d.", tmp_path, path, _errno);
            goto err;
        }
    }

    // Fsync dir
    {
        int e;
        do {
            e = fsync(dir_fd);
        } while (e == -1 && errno == EINTR);
        if (e < 0) {
            int _errno = errno;
            HAPAssert(e == -1);
            NVS_LOG_ERR("fsync of the directory %s failed: %d", gnvs_dir, _errno);
            goto err;
        }
    }

    HAPPlatformFileManagerCloseDirFreeSafe(dir);

    handle->changed = false;
    return true;

err1:
    close(tmp_fd);
    remove(tmp_path);
err:
    HAPPlatformFileManagerCloseDirFreeSafe(dir);
    return false;
}

void pal_nvs_close(pal_nvs_handle *handle) {
    HAPPrecondition(handle);

    if (handle->using_count > 1) {
        handle->using_count--;
        return;
    }
    pal_nvs_commit(handle);
    LIST_REMOVE(handle, list_entry);
    pal_nvs_remove_all_items(handle);
    pal_mem_free(handle->name);
    pal_mem_free(handle);
}
