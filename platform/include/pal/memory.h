// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_MEMORY_H_
#define PLATFORM_INCLUDE_PAL_MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Allocate and free dynamic memory.
 * Every platform must to implement it.
 */

/**
 * Allocate size bytes and return a pointer to the allocated memory.
 * The memory is not initialized.
 */
void *pal_mem_alloc(size_t size);

/**
 * Allocate memory size bytes and returns a pointer to the allocated memory.
 * The memory is set to zero.
 */
void *pal_mem_calloc(size_t size);

/**
 * Change the size of the memory block pointed to by ptr to size bytes.
 */
void *pal_mem_realloc(void *ptr, size_t size);

/**
 * Free the memory space pointed to by ptr, which must have been
 * returned by a previous call to pal_mem_alloc(), pal_mem_calloc(),
 * or pal_mem_realloc().
 */
void pal_mem_free(void *p);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_MEMORY_H_
