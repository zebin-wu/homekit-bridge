// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <embedfs.h>

static const embedfs_dir *embedfs_subdir(const embedfs_dir *dir, const char *name) {
    int left = 0;
    int right = dir->child_count - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        const embedfs_dir *child = dir->children[mid];
        int cmp = strcmp(child->name, name);
        if (cmp > 0) {
            right = mid - 1;
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            return child;
        }
    }
    return NULL;
}

const embedfs_file *embedfs_find_file(const embedfs_dir *dir, const char *path) {
    size_t len = strlen(path);
    char tmp[len + 1];
    memcpy(tmp, path, len);
    tmp[len] = '\0';

    char *start = tmp;
    char *end = start;

    while (*end) {
        if (*end == '/') {
            *end = '\0';
            dir = embedfs_subdir(dir, start);
            if (!dir) {
                return NULL;
            }
            start = end + 1;
            end = start;
        } else {
            end++;
        }
    }

    if (start == end) {
        return NULL;
    }

    int left = 0;
    int right = dir->file_count - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        const embedfs_file *file = dir->files[mid];
        int cmp = strcmp(file->name, start);
        if (cmp > 0) {
            right = mid - 1;
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            return file;
        }
    }
    return NULL;
}
