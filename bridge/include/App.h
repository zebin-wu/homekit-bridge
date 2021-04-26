// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <HAP.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Application lua Entry.
 *
 * @param workDir the path of the work directory. 
 * @return attribute count.
 */
size_t AppLuaEntry(const char *workDir);

/**
 * Close the lua state.
 */
void AppLuaClose(void);

/**
 * Initialize the application.
 */
void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Deinitialize the application.
 */
void AppRelease(void);

/**
 * Start the accessory server for the app.
 */
void AppAccessoryServerStart(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context);

void AccessoryServerHandleSessionAccept(HAPAccessoryServerRef* server, HAPSessionRef* session, void* _Nullable context);

void AccessoryServerHandleSessionInvalidate(
        HAPAccessoryServerRef* server,
        HAPSessionRef* session,
        void* _Nullable context);

/**
 * Restore platform specific factory settings.
 */
void RestorePlatformFactorySettings(void);

/**
 * Initialize App.
 */
void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks,
        void* _Nonnull * _Nonnull pcontext);

/**
 * De-initialize App.
 */
void AppDeinitialize();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
