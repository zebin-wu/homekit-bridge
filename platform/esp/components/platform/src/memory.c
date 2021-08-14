// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdlib.h>
#include <string.h>
#include <pal/memory.h>

void *pal_mem_alloc(size_t size)
{
    return malloc(size);
}

void *pal_mem_calloc(size_t size)
{ 
    return calloc(1, size);;
}

void *pal_mem_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void pal_mem_free(void *p)
{
    return free(p);
}
