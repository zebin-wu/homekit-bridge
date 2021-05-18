// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_INCLUDE_APP_H_
#define BRIDGE_INCLUDE_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAP.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Default lua entry script name.
#define BRIDGE_LUA_ENTRY_DEFAULT "main"

/**
 * Run the application lua entry.
 *
 * @param dir the path of the scripts directory.
 * @param entry the name of the entry script.
 * @return attribute count.
 */
size_t app_lua_run(const char *dir, const char *entry);

/**
 * Close the lua state.
 */
void app_lua_close(void);

/**
 * Initialize the application.
 */
void app_create(HAPAccessoryServerRef *server, HAPPlatformKeyValueStoreRef kv_store);

/**
 * Deinitialize the application.
 */
void app_release(void);

/**
 * Start the accessory server for the app.
 */
void app_accessory_server_start(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void app_accessory_server_handle_update_state(HAPAccessoryServerRef *server, void *_Nullable context);

/**
 * Handle the session accept of the Accessory Server.
 */
void app_accessory_server_handle_session_accept(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context);

/**
 * Handle the session invalidate of the Accessory Server.
 */
void app_accessory_server_handle_session_invalidate(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context);

/**
 * Initialize App.
 */
void app_init(
        HAPAccessoryServerOptions *server_opts,
        HAPPlatform *platform,
        HAPAccessoryServerCallbacks *server_cbs,
        void *_Nonnull *_Nonnull pcontext);

/**
 * De-initialize App.
 */
void app_deinit();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_INCLUDE_APP_H_
