// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_INCLUDE_APP_H_
#define BRIDGE_INCLUDE_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/err.h>
#include <HAP.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define APP_EXEC_DEFAULT_CMD "main"
#define APP_EXEC_DEFAULT_ARGC 0
#define APP_EXEC_DEFAULT_ARGV NULL
#define APP_CMD_ENTRY "main"

/**
 * Initialize App.
 *
 * @param workdir The path of the working directory.
 */
void app_init(const char *dir);

/**
 * De-initialize App.
 */
void app_deinit();

/**
 * @brief A callback called when the command is returned.
 *
 * @param err Error number, PAL_ERR_OK on success, otherwise on failure.
 * @param arg The last paramter of @b app_exec().
 */
typedef void (*app_exec_returned_cb)(pal_err err, void *arg);

/**
 * Execute a command.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param cb A callback called if the command return.
 * @param cb_arg The value to be passed as the last argument to @p cb.
 */
void app_exec(const char *cmd, size_t argc, const char *argv[], app_exec_returned_cb cb, void *cb_arg);

/**
 * Exit App.
 */
void app_exit();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_INCLUDE_APP_H_
