// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdlib.h>
#include <esp_attr.h>
#include <pal/memory.h>

IRAM_ATTR void *pal_mem_alloc(size_t size) {
    return malloc(size);
}

IRAM_ATTR void *pal_mem_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

IRAM_ATTR void *pal_mem_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

IRAM_ATTR void pal_mem_free(void *p) {
    free(p);
}
