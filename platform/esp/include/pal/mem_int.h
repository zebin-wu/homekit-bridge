// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_ESP_INCLUDE_PAL_MEM_INT_H
#define PLATFORM_ESP_INCLUDE_PAL_MEM_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/**
 * Allocate size bytes and return a pointer to the allocated memory.
 * The memory is not initialized.
 */
#define pal_mem_alloc(size) malloc(size)

/**
 * Allocates memory for an array of nmemb elements of size
 * bytes each and returns a pointer to the allocated memory.
 * The memory is set to zero.
 */
#define pal_mem_calloc(nmemb, size) calloc(nmemb, size)

/**
 * Change the size of the memory block pointed to by ptr to size bytes.
 *
 * If ptr is NULL, then the call is equivalent to pal_mem_alloc(size), for all values of size.
 * if size is equal to 0, and ptr is not NULL, then the call is equivalent to pal_mem_free(ptr).
 * If realloc() fails, the original block is left untouched; it is not freed or moved.
 */
#define pal_mem_realloc(ptr, size) realloc(ptr, size)

/**
 * Free the memory space pointed to by ptr, which must have been
 * returned by a previous call to pal_mem_alloc(), pal_mem_calloc(),
 * or pal_mem_realloc().
 */
#define pal_mem_free(ptr) free(ptr)

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_ESP_INCLUDE_PAL_MEM_INT_H
