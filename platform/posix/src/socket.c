// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pal/socket.h>
#include <pal/memory.h>

#include <HAPLog.h>
#include <HAPPlatform.h>
#include <HAPPlatformFileHandle.h>

/**
 * Log with level.
 *
 * @param level [Debug|Info|Default|Error|Fault]
 * @param socket The pointer to socket.
 */
#define SOCKET_LOG(level, obj, fmt, arg...) \
    HAPLogWithType(&socket_log_obj, kHAPLogType_ ## level, \
    "(%s:%u) " fmt, pal_socket_type_strs[(obj)->type], (obj)->id, ##arg)

#define SOCKET_LOG_ERRNO(socket, func) \
    SOCKET_LOG(Error, socket, "%s: %s() failed: %s.", __func__, func, strerror(errno))

typedef enum {
    PAL_SOCKET_CONN_ST_DISCONNECTED,
    PAL_SOCKET_CONN_ST_CONNECTING,
    PAL_SOCKET_CONN_ST_CONNECTED
} pal_socket_connection_state;

struct pal_socket_obj {
    bool bound:1;
    bool listened:1;
    pal_socket_connection_state conn_state;

    pal_socket_type type;
    pal_socket_domain domain;
    uint16_t id;
    int fd;

    const char *remote_addr;
    uint16_t remote_port;

    pal_socket_connected_cb connected_cb;
    void *connected_cb_arg;

    HAPPlatformFileHandleRef handle;
};

static const char *pal_socket_type_strs[] = {
    [PAL_SOCKET_TYPE_TCP] = "TCP",
    [PAL_SOCKET_TYPE_UDP] = "UDP"
};

static const size_t pal_socket_addr_lens[] = {
    [PAL_SOCKET_TYPE_TCP] = INET_ADDRSTRLEN,
    [PAL_SOCKET_TYPE_UDP] = INET6_ADDRSTRLEN,
};

static const HAPLogObject socket_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "socket",
};

static uint16_t gsocket_count;

static bool
pal_socket_addr_get_ipv4(struct sockaddr_in *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin_family = AF_INET;
    int ret = inet_pton(AF_INET, src_addr, &dst_addr->sin_addr.s_addr);
    if (ret <= 0) {
        return false;
    }
    dst_addr->sin_port = htons(port);
    return true;
}

static bool
pal_socket_addr_get_ipv6(struct sockaddr_in6 *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin6_family = AF_INET6;
    int ret = inet_pton(AF_INET6, src_addr, &dst_addr->sin6_addr);
    if (ret <= 0) {
        return false;
    }
    dst_addr->sin6_port = htons(port);
    return true;
}

pal_socket_obj *pal_socket_create(pal_socket_type type, pal_socket_domain domain) {
    pal_socket_obj *o = pal_mem_calloc(sizeof(*o));
    if (!o) {
        HAPLogWithType(&socket_log_obj, kHAPLogType_Error, "%s: Failed to calloc memory.", __func__);
        return NULL;
    }

    int _domain, _type, _protocol;

    switch (domain) {
    case PAL_SOCKET_DOMAIN_IPV4:
        _domain = AF_INET;
        break;
    case PAL_SOCKET_DOMAIN_IPV6:
        _domain = AF_INET6;
        break;
    default:
        HAPAssertionFailure();
    }
    switch (type) {
    case PAL_SOCKET_TYPE_TCP:
        _type = SOCK_STREAM;
        _protocol = IPPROTO_TCP;
        break;
    case PAL_SOCKET_TYPE_UDP:
        _type = SOCK_DGRAM;
        _protocol = IPPROTO_UDP;
        break;
    default:
        HAPAssertionFailure();
    }

    o->fd = socket(_domain, _type, _protocol);
    if (o->fd == -1) {
        SOCKET_LOG_ERRNO(o, "socket");
        pal_mem_free(o);
        return NULL;
    }

    if (fcntl(o->fd, F_SETFL, fcntl(o->fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        SOCKET_LOG_ERRNO(o, "fcntl");
        pal_mem_free(o);
        return NULL;
    }

    o->type = type;
    o->domain = domain;
    o->id = ++gsocket_count;

    HAPError err = HAPPlatformFileHandleRegister(&o->handle, o->fd, (HAPPlatformFileHandleEvent) {
        .isReadyForReading = false,
        .isReadyForWriting = false,
        .hasErrorConditionPending = false
    }, NULL, NULL);
    if (err != kHAPError_None) {
        SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
        return NULL;
    }

    SOCKET_LOG(Debug, o, "%s(%p)", __func__, o);
    return o;
}

void pal_socket_destroy(pal_socket_obj *o) {
    if (!o) {
        return;
    }
    SOCKET_LOG(Debug, o, "%s(%p)", __func__, o);
    if (o->remote_addr) {
        pal_mem_free(o->remote_addr);
    }
    close(o->fd);
    pal_mem_free(o);
}

pal_socket_err pal_socket_bind(pal_socket_obj *o, const char *addr, uint16_t port) {
    HAPPrecondition(o);
    HAPPrecondition(addr);

    int ret;
    switch (o->domain) {
    case PAL_SOCKET_DOMAIN_IPV4: {
        struct sockaddr_in sa;
        if (!pal_socket_addr_get_ipv4(&sa, addr, port)) {
            SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_SOCKET_ERR_INVALID_ARG;
        }
        ret = bind(o->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    case PAL_SOCKET_DOMAIN_IPV6: {
        struct sockaddr_in6 sa;
        if (!pal_socket_addr_get_ipv6(&sa, addr, port)) {
            SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_SOCKET_ERR_INVALID_ARG;
        }
        ret = bind(o->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    default:
        HAPAssertionFailure();
    }
    if (ret == -1) {
        SOCKET_LOG_ERRNO(o, "bind");
        return PAL_SOCKET_ERR_UNKNOWN;
    }
    o->bound = true;
    SOCKET_LOG(Debug, o, "Bound to %s:%u", addr, port);
    return PAL_SOCKET_ERR_OK;
}

static pal_socket_err pal_socket_connect_async(pal_socket_obj *o, const char *addr, uint16_t port) {
    int rc;
    switch (o->domain) {
    case PAL_SOCKET_DOMAIN_IPV4: {
        struct sockaddr_in sa;
        if (!pal_socket_addr_get_ipv4(&sa, addr, port)) {
            SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_SOCKET_ERR_INVALID_ARG;
        }
        do {
            rc = connect(o->fd, (struct sockaddr *)&sa, sizeof(sa));
        } while (rc == -1 && errno == EINTR);
        break;
    }
    case PAL_SOCKET_DOMAIN_IPV6: {
        struct sockaddr_in6 sa;
        if (!pal_socket_addr_get_ipv6(&sa, addr, port)) {
            SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_SOCKET_ERR_INVALID_ARG;
        }
        do {
            rc = connect(o->fd, (struct sockaddr *)&sa, sizeof(sa));
        } while (rc == -1 && errno == EINTR);
        break;
    }
    default:
        HAPAssertionFailure();
    }
    if (rc == -1) {
        if (errno == EINPROGRESS) {
            return PAL_SOCKET_ERR_IN_PROGRESS;
        } else {
            return PAL_SOCKET_ERR_UNKNOWN;
        }
    }

    SOCKET_LOG(Debug, o, "Connected to %s:%u", addr, port);
    return PAL_SOCKET_ERR_OK;
}

static void pal_socket_handle_connection_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj *o = context;
    pal_socket_err err;

    if (fileHandleEvents.hasErrorConditionPending) {
        err = PAL_SOCKET_ERR_UNKNOWN;
    } else if (fileHandleEvents.isReadyForReading) {
        err = pal_socket_connect_async(o, o->remote_addr, o->remote_port);
        switch (err) {
        case PAL_SOCKET_ERR_OK:
            o->conn_state = PAL_SOCKET_CONN_ST_CONNECTED;
        default:
            HAPPlatformFileHandleUpdateInterests(o->handle, (HAPPlatformFileHandleEvent) {
                .isReadyForReading = false,
                .isReadyForWriting = false,
                .hasErrorConditionPending = false
            }, NULL, NULL);
            break;
        }
    }

    if (o->connected_cb) {
        o->connected_cb(o, err, o->connected_cb_arg);
    }
}

pal_socket_err pal_socket_connect(pal_socket_obj *o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg) {
    size_t addr_len = strlen(addr);
    HAPPrecondition(o);
    HAPPrecondition(addr);
    HAPPrecondition(addr_len < pal_socket_addr_lens[o->type]);
    HAPPrecondition(o->listened == false);

    if (o->conn_state != PAL_SOCKET_CONN_ST_DISCONNECTED) {
        SOCKET_LOG(Error, o, "%s: Invalid connection state.", __func__);
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    pal_socket_err err = pal_socket_connect_async(o, addr, port);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        o->conn_state = PAL_SOCKET_CONN_ST_CONNECTING;
        o->connected_cb = connected_cb;
        o->connected_cb_arg = arg;
        HAPPlatformFileHandleUpdateInterests(o->handle, (HAPPlatformFileHandleEvent) {
            .isReadyForReading = true,
            .isReadyForWriting = false,
            .hasErrorConditionPending = true
        }, pal_socket_handle_connection_cb, o);
        SOCKET_LOG(Debug, o, "Connecting to %s:%u ...", addr, port);
    case PAL_SOCKET_ERR_OK:
        o->conn_state = PAL_SOCKET_CONN_ST_CONNECTED;
        o->remote_addr = pal_mem_alloc(addr_len + 1);
        memcpy(o->remote_addr, addr, addr_len + 1);
        o->remote_port = port;
        break;
    default:
        break;
    }

    return err;
}
