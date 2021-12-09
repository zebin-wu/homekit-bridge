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
    PAL_SOCKET_ST_NONE,
    PAL_SOCKET_ST_CONNECTING,
    PAL_SOCKET_ST_CONNECTED,
    PAL_SOCKET_ST_LISTENED,
    PAL_SOCKET_ST_ACCEPTING
} pal_socket_state;

typedef union {
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
} pal_socket_addr;

struct pal_socket_obj {
    pal_socket_state state;

    pal_socket_type type;
    pal_socket_domain domain;
    uint16_t id;
    int fd;

    pal_socket_addr remote_addr;

    pal_socket_connected_cb connected_cb;
    void *connected_cb_arg;

    pal_socket_accepted_cb accepted_cb;
    void *accepted_cb_arg;

    HAPPlatformFileHandleRef handle;
};

static const char *pal_socket_type_strs[] = {
    [PAL_SOCKET_TYPE_TCP] = "TCP",
    [PAL_SOCKET_TYPE_UDP] = "UDP"
};

static const HAPLogObject socket_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "socket",
};

static uint16_t gsocket_count;

static bool
pal_socket_addr_set(pal_socket_addr *addr, pal_socket_domain domain, const char *str_addr, uint16_t port) {
    switch (domain) {
    case PAL_SOCKET_DOMAIN_INET: {
        struct sockaddr_in *sa = &addr->in;
        sa->sin_family = AF_INET;
        int ret = inet_pton(AF_INET, str_addr, &sa->sin_addr);
        if (ret <= 0) {
            return false;
        }
        sa->sin_port = htons(port);
        break;
    }
    case PAL_SOCKET_DOMAIN_INET6: {
        struct sockaddr_in6 *sa = &addr->in6;
        sa->sin6_family = AF_INET6;
        int ret = inet_pton(AF_INET6, str_addr, &sa->sin6_addr);
        if (ret <= 0) {
            return false;
        }
        sa->sin6_port = htons(port);
        break;
    }
    default:
        HAPAssertionFailure();
        break;
    }

    return true;
}

static inline size_t
pal_socket_addr_get_len(pal_socket_addr *addr) {
    switch (((struct sockaddr *)addr)->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        HAPAssertionFailure();
        break;
    }
    return 0;
}

static inline uint16_t
pal_socket_addr_get_port(pal_socket_addr *addr) {
    switch (((struct sockaddr *)addr)->sa_family) {
    case AF_INET:
        return ntohs(addr->in.sin_port);
    case AF_INET6:
        return ntohs(addr->in6.sin6_port);
    default:
        HAPAssertionFailure();
        break;
    }
    return 0;
}

static inline const char *
pal_socket_addr_get_str_addr(pal_socket_addr *addr, char *buf, size_t buflen) {
    switch (((struct sockaddr *)addr)->sa_family) {
    case AF_INET:
        return inet_ntop(AF_INET, &addr->in.sin_addr, buf, buflen);
    case AF_INET6:
        return inet_ntop(AF_INET6, &addr->in6.sin6_addr, buf, buflen);
    default:
        HAPAssertionFailure();
        break;
    }
    return NULL;
}

static bool pal_socket_set_nonblock(pal_socket_obj *o) {
    if (fcntl(o->fd, F_SETFL, fcntl(o->fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        SOCKET_LOG_ERRNO(o, "fcntl");
        return false;
    }
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
    case PAL_SOCKET_DOMAIN_INET:
        _domain = AF_INET;
        break;
    case PAL_SOCKET_DOMAIN_INET6:
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
        goto err;
    }

    if (!pal_socket_set_nonblock(o)) {
        goto err;
    }

    o->type = type;
    o->domain = domain;
    o->id = ++gsocket_count;

    SOCKET_LOG(Debug, o, "%s(%p)", __func__, o);
    return o;

err:
    pal_mem_free(o);
    return NULL;
}

void pal_socket_destroy(pal_socket_obj *o) {
    if (!o) {
        return;
    }
    SOCKET_LOG(Debug, o, "%s(%p)", __func__, o);
    close(o->fd);
    if (o->handle) {
        HAPPlatformFileHandleDeregister(o->handle);
        o->handle = 0;
    }
    pal_mem_free(o);
}

pal_socket_err pal_socket_bind(pal_socket_obj *o, const char *addr, uint16_t port) {
    HAPPrecondition(o);
    HAPPrecondition(addr);

    int ret;
    pal_socket_addr sa;

    if (!pal_socket_addr_set(&sa, o->domain, addr, port)) {
        SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
        return PAL_SOCKET_ERR_INVALID_ARG;
    }

    ret = bind(o->fd, (struct sockaddr *)&sa, pal_socket_addr_get_len(&sa));
    if (ret == -1) {
        SOCKET_LOG_ERRNO(o, "bind");
        return PAL_SOCKET_ERR_UNKNOWN;
    }
    SOCKET_LOG(Debug, o, "Bound to %s:%u", addr, port);
    return PAL_SOCKET_ERR_OK;
}

pal_socket_err pal_socket_listen(pal_socket_obj *o, int backlog) {
    HAPPrecondition(o);

    if (o->state != PAL_SOCKET_ST_NONE) {
        SOCKET_LOG(Error, o, "%s: Invalid state.", __func__);
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    int ret = listen(o->fd, backlog);
    if (ret == -1) {
        SOCKET_LOG_ERRNO(o, "listen");
        return PAL_SOCKET_ERR_UNKNOWN;
    }
    o->state = PAL_SOCKET_ST_LISTENED;
    return PAL_SOCKET_ERR_OK;
}

static pal_socket_err pal_socket_accept_async(pal_socket_obj *o, pal_socket_obj **new_o) {
    int new_fd;
    pal_socket_addr addr;
    socklen_t addrlen = sizeof(addr);

    do {
        new_fd = accept(o->fd, (struct sockaddr *)&addr, &addrlen);
    } while (new_fd == -1 && errno == EINTR);
    if (new_fd == -1) {
        if (errno == EAGAIN) {
            return PAL_SOCKET_ERR_IN_PROGRESS;
        } else {
            SOCKET_LOG_ERRNO(o, "accept");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
    }

    pal_socket_obj *_new = pal_mem_calloc(sizeof(pal_socket_obj));
    if (!_new) {
        SOCKET_LOG(Error, o, "%s: Failed to calloc memory.", __func__);
        return PAL_SOCKET_ERR_ALLOC;
    }

    _new->fd = new_fd;
    _new->type = o->type;
    _new->domain = o->domain;
    _new->id = ++gsocket_count;
    _new->state = PAL_SOCKET_ST_CONNECTED;
    _new->remote_addr = addr;

    if (!pal_socket_set_nonblock(_new)) {
        close(new_fd);
        pal_mem_free(_new);
        return PAL_SOCKET_ERR_UNKNOWN;
    }

    *new_o = _new;
    return PAL_SOCKET_ERR_OK;
}

static void pal_socket_handle_accept_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj *o = context;
    pal_socket_obj *new_o = NULL;
    pal_socket_err err = PAL_SOCKET_ERR_OK;

    if (fileHandleEvents.hasErrorConditionPending) {
        err = PAL_SOCKET_ERR_UNKNOWN;
        o->state = PAL_SOCKET_ST_LISTENED;
    } else if (fileHandleEvents.isReadyForWriting) {
        err = pal_socket_accept_async(o, &new_o);
        switch (err) {
        case PAL_SOCKET_ERR_OK: {
            char buf[64];
            SOCKET_LOG(Debug, o, "Accept a connection from %s:%d",
                pal_socket_addr_get_str_addr(&new_o->remote_addr, buf, sizeof(buf)),
                pal_socket_addr_get_port(&new_o->remote_addr));
        }
        default:
            o->state = PAL_SOCKET_ST_LISTENED;
            break;
        }
    }
    HAPPlatformFileHandleDeregister(o->handle);
    o->handle = 0;

    if (o->accepted_cb) {
        o->accepted_cb(o, err, new_o, o->accepted_cb_arg);
    }
}

pal_socket_err
pal_socket_accept(pal_socket_obj *o, pal_socket_obj **new_o, pal_socket_accepted_cb accepted_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(new_o);
    HAPPrecondition(accepted_cb);

    if (o->state != PAL_SOCKET_ST_LISTENED) {
        SOCKET_LOG(Error, o, "%s: Invalid state.", __func__);
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    pal_socket_err err = pal_socket_accept_async(o, new_o);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        if (HAPPlatformFileHandleRegister(&o->handle, o->fd, (HAPPlatformFileHandleEvent) {
            .isReadyForReading = true,
            .isReadyForWriting = false,
            .hasErrorConditionPending = true
        }, pal_socket_handle_accept_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
            return PAL_SOCKET_ERR_UNKNOWN;
        }
        o->state = PAL_SOCKET_ST_ACCEPTING;
        o->accepted_cb = accepted_cb;
        o->accepted_cb_arg = arg;
        SOCKET_LOG(Debug, o, "Accepting ...");
        break;
    case PAL_SOCKET_ERR_OK: {
        char buf[64];
        SOCKET_LOG(Debug, o, "Accept a connection from %s:%d",
            pal_socket_addr_get_str_addr(&(*new_o)->remote_addr, buf, sizeof(buf)),
            pal_socket_addr_get_port(&(*new_o)->remote_addr));
        break;
    }
    default:
        break;
    }

    return err;
}

static pal_socket_err pal_socket_connect_async(pal_socket_obj *o) {
    int ret;

    do {
        ret = connect(o->fd, (struct sockaddr *)&o->remote_addr,
            pal_socket_addr_get_len(&o->remote_addr));
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            return PAL_SOCKET_ERR_IN_PROGRESS;
        } else {
            SOCKET_LOG_ERRNO(o, "connect");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
    }

    return PAL_SOCKET_ERR_OK;
}

static void pal_socket_handle_connect_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj *o = context;
    pal_socket_err err = PAL_SOCKET_ERR_OK;

    if (fileHandleEvents.hasErrorConditionPending) {
        err = PAL_SOCKET_ERR_UNKNOWN;
        o->state = PAL_SOCKET_ST_NONE;
    } else if (fileHandleEvents.isReadyForWriting) {
        err = pal_socket_connect_async(o);
        switch (err) {
        case PAL_SOCKET_ERR_OK: {
            char buf[64];
            o->state = PAL_SOCKET_ST_CONNECTED;
            SOCKET_LOG(Debug, o, "Connected to %s:%u",
                pal_socket_addr_get_str_addr(&o->remote_addr, buf, sizeof(buf)),
                pal_socket_addr_get_port(&o->remote_addr));
            break;
        }
        default:
            o->state = PAL_SOCKET_ST_NONE;
            break;
        }
    }
    HAPPlatformFileHandleDeregister(o->handle);
    o->handle = 0;

    if (o->connected_cb) {
        o->connected_cb(o, err, o->connected_cb_arg);
    }
}

pal_socket_err pal_socket_connect(pal_socket_obj *o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(addr);
    HAPPrecondition(connected_cb);

    if (o->state != PAL_SOCKET_ST_NONE) {
        SOCKET_LOG(Error, o, "%s: Invalid state.", __func__);
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    if (!pal_socket_addr_set(&o->remote_addr, o->domain, addr, port)) {
        SOCKET_LOG(Error, o, "%s: Invalid address \"%s\".", __func__, addr);
        return PAL_SOCKET_ERR_INVALID_ARG;
    }

    pal_socket_err err = pal_socket_connect_async(o);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        if (HAPPlatformFileHandleRegister(&o->handle, o->fd, (HAPPlatformFileHandleEvent) {
            .isReadyForReading = false,
            .isReadyForWriting = true,
            .hasErrorConditionPending = true
        }, pal_socket_handle_connect_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
            return PAL_SOCKET_ERR_UNKNOWN;
        }
        o->state = PAL_SOCKET_ST_CONNECTING;
        o->connected_cb = connected_cb;
        o->connected_cb_arg = arg;
        SOCKET_LOG(Debug, o, "Connecting to %s:%u ...", addr, port);
        break;
    case PAL_SOCKET_ERR_OK:
        o->state = PAL_SOCKET_ST_CONNECTED;
        SOCKET_LOG(Debug, o, "Connected to %s:%u", addr, port);
        break;
    default:
        break;
    }

    return err;
}
