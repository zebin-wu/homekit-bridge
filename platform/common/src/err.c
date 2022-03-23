// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/err.h>
#include <HAPBase.h>

const char *err_strs[] = {
    [PAL_ERR_OK] = "no error",
    [PAL_ERR_TIMEOUT] = "timeout",
    [PAL_ERR_IN_PROGRESS] = "in progress",
    [PAL_ERR_UNKNOWN] = "unknown error",
    [PAL_ERR_ALLOC] = "failed to alloc",
    [PAL_ERR_INVALID_ARG] = "invalid argument",
    [PAL_ERR_INVALID_STATE] = "invalid state",
    [PAL_ERR_BUSY] = "resource busy",
    [PAL_ERR_AGAIN] = "try again",
};

const char *pal_err_string(pal_err err) {
    HAPPrecondition(err >= PAL_ERR_OK && err < PAL_ERR_COUNT);
    return err_strs[err];
}
