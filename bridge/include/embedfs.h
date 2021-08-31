// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_INCLUDE_EMBEDFS_H_
#define BRIDGE_INCLUDE_EMBEDFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * File description.
 */
typedef struct embedfs_file embedfs_file;

/**
 * Directory description.
 */
typedef struct embedfs_dir embedfs_dir;

struct embedfs_file {
    const char *name;       /**< File name. */
    const char *data;       /**< File data. */
    size_t len;            /**< Data length in bytes. */
};

struct embedfs_dir {
    const char *name;
    const embedfs_file * const *files;
    const struct embedfs_dir * const *children;
};

const embedfs_file *embedfs_find_file(const embedfs_dir *dir, const char *path);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_INCLUDE_EMBEDFS_H_
