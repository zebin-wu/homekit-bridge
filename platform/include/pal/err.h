// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_ERR_H_
#define PLATFORM_INCLUDE_PAL_ERR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Platform common error.
 * Every platform must to implement it.
 */

/**
 * Error numbers.
 */
typedef enum pal_err {
    PAL_ERR_OK,             /**< no error */
    PAL_ERR_TIMEOUT,        /**< timeout */
    PAL_ERR_IN_PROGRESS,    /**< in progress */
    PAL_ERR_UNKNOWN,        /**< unknown */
    PAL_ERR_ALLOC,          /**< failed to alloc */
    PAL_ERR_INVALID_ARG,    /**< invalid argument */
    PAL_ERR_INVALID_STATE,  /**< invalid state */
    PAL_ERR_BUSY,           /**< resource busy */
    PAL_ERR_AGAIN,          /**< try again */
    PAL_ERR_WANT_READ,      /**< want read */
    PAL_ERR_WANT_WRITE,     /**< want write */
    PAL_ERR_NOT_FOUND,      /**< not found */

    PAL_ERR_COUNT,          /**< Error count, not error number. */
} pal_err;

const char *pal_err_string(pal_err err);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_ERR_H_
