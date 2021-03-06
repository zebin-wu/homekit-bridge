// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.
//
// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HAP_PLATFORM_INIT_H
#define HAP_PLATFORM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optional features set in Makefile.
 */
/**@{*/
#ifndef HAVE_DISPLAY
#define HAVE_DISPLAY 0
#endif

#ifndef HAVE_NFC
#define HAVE_NFC 0
#endif

#ifdef CONFIG_HAP_MFI_HW_AUTH
#define HAVE_MFI_HW_AUTH 1
#else
#define HAVE_MFI_HW_AUTH 0
#endif
/**@}*/

#include <stdlib.h>

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Deallocate memory pointed to by ptr.
 *
 * @param      ptr                  Pointer to memory to be deallocated.
 */
#define HAPPlatformFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        free(ptr); \
        ptr = NULL; \
    } while (0)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
