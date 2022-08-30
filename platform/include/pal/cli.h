// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CLI_H_
#define PLATFORM_INCLUDE_PAL_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/err.h>

/**
 * command-line interface.
 */

/**
 * Command information.
 */
typedef struct {
    /**
     * Command name. Must not be NULL, must not contain spaces.
     * The pointer must be valid until deinit.
     */
    const char *cmd;

    /**
     * Help text for the command, shown by help command.
     * If set, the pointer must be valid until deinit.
     * If not set, the command will not be listed in 'help' output.
     */
    const char *help;

    /**
     * Hint text, usually lists possible arguments.
     * If set, the pointer must be valid until deinit.
     */
    const char *hint;

    /**
     * Pointer to a function which implements the command.
     */
    int (*func)(int argc, char *argv[]);
} pal_cli_info;

/**
 * Register a command.
 *
 * @param info The command information.
 *
 * @return PAL_ERR_OK on success.
 * @return other error number on failure.
 */
pal_err pal_cli_register(const pal_cli_info *info);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CLI_H_
