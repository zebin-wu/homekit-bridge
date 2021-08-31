// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <HAPBase.h>
#include <embedfs.h>

static const embedfs_dir *embedfs_subdir(const embedfs_dir *dir, const char *name) {
    for (const embedfs_dir * const *pchild = dir->children; *pchild; pchild++) {
        if (HAPStringAreEqual((*pchild)->name, name)) {
            return *pchild;
        }
    }
    return NULL;
}

const embedfs_file *embedfs_find_file(const embedfs_dir *dir, const char *path) {
    size_t len = HAPStringGetNumBytes(path);
    char tmp[len + 1];
    HAPRawBufferCopyBytes(tmp, path, len);
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

    for (const embedfs_file * const *pfile = dir->files; *pfile; pfile++) {
        if (HAPStringAreEqual((*pfile)->name, start)) {
            return *pfile;
        }
    }

    return NULL;
}
