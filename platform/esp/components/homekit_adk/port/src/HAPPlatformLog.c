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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>

#include "HAP.h"
#include "HAPPlatformLog+Init.h"

#if _POSIX_C_SOURCE >= 200112L && ! _GNU_SOURCE
#error "This file needs the GNU-specific version of 'strerror_r'."
#endif

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Log" };

void HAPPlatformLogPOSIXError(
        HAPLogType type,
        const char* _Nonnull message,
        int errorNumber,
        const char* _Nonnull function,
        const char* _Nonnull file,
        int line) {
    HAPPrecondition(message);
    HAPPrecondition(function);
    HAPPrecondition(file);

    // Get error message.
    char errorStringBuffer[256];
    char *errorString = strerror_r(errorNumber, errorStringBuffer, sizeof errorStringBuffer);

    // Perform logging.
    HAPLogWithType(&logObject, type, "%s:%d:%s - %s @ %s:%d", message, errorNumber, errorString, function, file, line);
}

static void getTag(const HAPLogObject* log, char *buf, size_t buflen) {
    if (log->subsystem) {
        int len;
        len = snprintf(buf, buflen, "%s", log->subsystem);
        if (log->category) {
            len += snprintf(buf + len, buflen - len, ":%s", log->category);
        }
    } else {
        snprintf(buf, buflen, kHAPPlatform_LogSubsystem);
    }
}

HAP_RESULT_USE_CHECK
HAPPlatformLogEnabledTypes HAPPlatformLogGetEnabledTypes(const HAPLogObject* _Nonnull log HAP_UNUSED) {
    char tag[64];
    getTag(log, tag, sizeof(tag));

    switch (esp_log_level_get(tag)) {
        case ESP_LOG_NONE:
            return kHAPPlatformLogEnabledTypes_None;
        case ESP_LOG_ERROR:
        case ESP_LOG_WARN:
            return kHAPPlatformLogEnabledTypes_Default;
        case ESP_LOG_INFO:
            return kHAPPlatformLogEnabledTypes_Info;
        case ESP_LOG_DEBUG:
        case ESP_LOG_VERBOSE:
            return kHAPPlatformLogEnabledTypes_Debug;
        default:
            HAPFatalError();
    }
}

HAP_PRINTFLIKE(5, 0)
void HAPPlatformLogCapture(
        const HAPLogObject* log,
        HAPLogType type,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        va_list args) HAP_DIAGNOSE_ERROR(!bufferBytes && numBufferBytes, "empty buffer cannot have a length") {
    HAPPrecondition(log);
    HAPPrecondition(!numBufferBytes || bufferBytes);

    char tag[64];
    getTag(log, tag, sizeof(tag));

    esp_log_level_t level;

    switch (type) {
    case kHAPLogType_Fault:
    case kHAPLogType_Error:
        level = ESP_LOG_ERROR;
        esp_log_write(level, tag, LOG_COLOR_E "E (%lu) %s: ", esp_log_timestamp(), tag);
        break;
    case kHAPLogType_Default:
        level = ESP_LOG_WARN;
        esp_log_write(level, tag, LOG_COLOR_W "W (%lu) %s: ", esp_log_timestamp(), tag);
        break;
    case kHAPLogType_Info:
        level = ESP_LOG_INFO;
        esp_log_write(level, tag, LOG_COLOR_I "I (%lu) %s: ", esp_log_timestamp(), tag);
        break;
    case kHAPLogType_Debug:
        level = ESP_LOG_DEBUG;
        esp_log_write(level, tag, LOG_COLOR_D "D (%lu) %s: ", esp_log_timestamp(), tag);
        break;
    default:
        HAPFatalError();
    }

    esp_log_writev(level, tag, format, args);
    esp_log_write(level, tag, LOG_RESET_COLOR "\n");
    esp_log_buffer_hexdump_internal(tag, bufferBytes, numBufferBytes, level);
}
