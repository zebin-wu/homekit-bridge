/**
 * Copyright (c) 2021 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
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
 * Application Lua Entry.
 *
 * @param workDir the path of the work directory. 
 * @return attribute count.
 */
size_t AppLuaEntry(const char *workDir);

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
