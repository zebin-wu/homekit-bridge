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
#include <HAPPlatformTimer.h>

/**
 * Log with level.
 *
 * @param level [Debug|Info|Default|Error|Fault]
 * @param socket The pointer to socket.
 */
#define SOCKET_LOG(level, obj, fmt, arg...) \
    HAPLogWithType(&socket_log_obj, kHAPLogType_ ## level, \
    "(%s:%u) " fmt, pal_socket_type_strs[(obj)->type], (obj)->id, ##arg)

#define SOCKET_LOGBUF(level, buf, len, obj, fmt, arg...) \
    HAPLogBuffer ## level(&socket_log_obj, buf, len, \
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

typedef struct pal_socket_mbuf {
    pal_socket_addr to_addr;
    pal_socket_sent_cb sent_cb;
    void *arg;
    struct pal_socket_mbuf *next;
    size_t len;
    size_t pos;
    char buf[0];
} pal_socket_mbuf;

struct pal_socket_obj {
    bool receiving;
    pal_socket_state state;

    pal_socket_type type;
    pal_socket_domain domain;
    uint16_t id;
    int fd;
    uint32_t timeout;
    HAPPlatformTimerRef timer;
    size_t recv_maxlen;

    pal_socket_addr remote_addr;

    pal_socket_connected_cb connected_cb;
    pal_socket_accepted_cb accepted_cb;
    pal_socket_recved_cb recved_cb;
    void *cb_arg;

    HAPPlatformFileHandleRef handle;
    HAPPlatformFileHandleEvent interests;

    pal_socket_mbuf *mbuf_list_head;
    pal_socket_mbuf **mbuf_list_ptail;
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

static void pal_socket_handle_event_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context);

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
pal_socket_addr_set_len(pal_socket_addr *addr) {
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

static pal_socket_mbuf *pal_socket_mbuf_create(const void *data, size_t len,
    pal_socket_addr *to_addr, pal_socket_sent_cb sent_cb, void *arg) {
    pal_socket_mbuf *mbuf = pal_mem_alloc(sizeof(*mbuf) + len);
    if (!mbuf) {
        return NULL;
    }

    if (to_addr) {
        mbuf->to_addr = *to_addr;
    } else {
        mbuf->to_addr.in.sin_family = AF_UNSPEC;
    }
    memcpy(mbuf->buf, data, len);
    mbuf->pos = 0;
    mbuf->len = len;
    mbuf->sent_cb = sent_cb;
    mbuf->arg = arg;

    return mbuf;
}

static void pal_socket_mbuf_in(pal_socket_obj *o, pal_socket_mbuf *mbuf) {
    mbuf->next = NULL;
    *(o->mbuf_list_ptail) = mbuf;
    o->mbuf_list_ptail = &mbuf->next;
}

static pal_socket_mbuf *pal_socket_mbuf_top(pal_socket_obj *o) {
    return o->mbuf_list_head;
}

static pal_socket_mbuf *pal_socket_mbuf_out(pal_socket_obj *o) {
    pal_socket_mbuf *mbuf = o->mbuf_list_head;
    if (mbuf) {
        o->mbuf_list_head = mbuf->next;
        if (o->mbuf_list_head == NULL) {
            o->mbuf_list_ptail = &o->mbuf_list_head;
        }
    }
    return mbuf;
}

static bool pal_socket_set_nonblock(pal_socket_obj *o) {
    if (fcntl(o->fd, F_SETFL, fcntl(o->fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        SOCKET_LOG_ERRNO(o, "fcntl");
        return false;
    }
    return true;
}

static pal_socket_err pal_socket_accept_async(pal_socket_obj *o, pal_socket_obj **new_o, pal_socket_addr *addr) {
    int new_fd;
    socklen_t addrlen = sizeof(*addr);

    do {
        new_fd = accept(o->fd, (struct sockaddr *)addr, &addrlen);
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
        return PAL_SOCKET_ERR_ALLOC;
    }

    _new->fd = new_fd;
    _new->type = o->type;
    _new->domain = o->domain;
    _new->id = ++gsocket_count;
    _new->state = PAL_SOCKET_ST_CONNECTED;
    _new->remote_addr = *addr;

    if (!pal_socket_set_nonblock(_new)) {
        close(new_fd);
        pal_mem_free(_new);
        return PAL_SOCKET_ERR_UNKNOWN;
    }

    _new->interests.isReadyForReading = true;
    if (HAPPlatformFileHandleRegister(&_new->handle, _new->fd, _new->interests,
        pal_socket_handle_event_cb, _new) != kHAPError_None) {
        SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
        return PAL_SOCKET_ERR_UNKNOWN;
    }

    *new_o = _new;
    return PAL_SOCKET_ERR_OK;
}

static pal_socket_err pal_socket_connect_async(pal_socket_obj *o) {
    int ret;

    do {
        ret = connect(o->fd, (struct sockaddr *)&o->remote_addr,
            pal_socket_addr_set_len(&o->remote_addr));
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

static pal_socket_err
pal_socket_send_async(pal_socket_obj *o, const void *data, size_t *len, pal_socket_addr *addr) {
    ssize_t rc;
    socklen_t addrlen = addr ? pal_socket_addr_set_len(addr) : 0;

    do {
        rc = sendto(o->fd, data, *len, 0, (struct sockaddr *)addr, addrlen);
    } while (rc == -1 && errno == EINTR);
    if (rc == -1) {
        *len = 0;
        if (errno == EAGAIN) {
            return PAL_SOCKET_ERR_IN_PROGRESS;
        } else {
            SOCKET_LOG_ERRNO(o, "sendto");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
    } else if (rc == 0) {
        *len = 0;
        return PAL_SOCKET_ERR_CLOSED;
    }
    if (rc != *len) {
        *len = rc;
        return PAL_SOCKET_ERR_IN_PROGRESS;
    }
    return PAL_SOCKET_ERR_OK;
}

static pal_socket_err
pal_socket_recv_async(pal_socket_obj *o, void *buf, size_t *len, pal_socket_addr *addr) {
    ssize_t rc;

    if (addr) {
        socklen_t addrlen = sizeof(*addr);
        do {
            rc = recvfrom(o->fd, buf, *len, 0, (struct sockaddr *)addr, &addrlen);
        } while (rc == -1 && errno == EINTR);
    } else {
        do {
            rc = recv(o->fd, buf, *len, 0);
        } while (rc == -1 && errno == EINTR);
    }
    if (rc == -1) {
        *len = 0;
        if (errno == EAGAIN) {
            return PAL_SOCKET_ERR_IN_PROGRESS;
        } else {
            SOCKET_LOG_ERRNO(o, "recvfrom");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
    }
    *len = rc;
    return PAL_SOCKET_ERR_OK;
}

static void pal_socket_handle_accept_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForReading) {
        return;
    }

    pal_socket_addr sa;
    pal_socket_obj *o = context;
    pal_socket_obj *new_o = NULL;

    if (o->state != PAL_SOCKET_ST_ACCEPTING) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    pal_socket_err err = pal_socket_accept_async(o, &new_o, &sa);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        break;
    case PAL_SOCKET_ERR_OK: {
        char addr[64];
        uint16_t port = pal_socket_addr_get_port(&new_o->remote_addr);
        pal_socket_addr_get_str_addr(&new_o->remote_addr, addr, sizeof(addr));
        SOCKET_LOG(Debug, o, "Accept a connection from %s:%d", addr, port);
        o->state = PAL_SOCKET_ST_LISTENED;
        if (o->accepted_cb) {
            o->accepted_cb(o, err, new_o, addr, port, o->cb_arg);
        }
        break;
    }
    default:
        o->state = PAL_SOCKET_ST_LISTENED;
        if (o->accepted_cb) {
            o->accepted_cb(o, err, new_o, NULL, 0, o->cb_arg);
        }
        break;
    }
}

static void pal_socket_handle_connect_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForWriting) {
        return;
    }

    pal_socket_obj *o = context;

    if (o->state != PAL_SOCKET_ST_CONNECTING) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    pal_socket_err err = pal_socket_connect_async(o);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        return;
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

    o->interests.isReadyForWriting = false;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);

    if (o->connected_cb) {
        o->connected_cb(o, err, o->cb_arg);
    }
}

static void pal_socket_handle_send_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForWriting) {
        return;
    }

    pal_socket_obj *o = context;
    pal_socket_mbuf *mbuf = pal_socket_mbuf_top(o);
    if (!mbuf) {
        return;
    }

    bool issendto = mbuf->to_addr.in.sin_family != AF_UNSPEC;
    size_t sent_len = mbuf->len - mbuf->pos;
    pal_socket_err err = pal_socket_send_async(o, mbuf->buf + mbuf->pos, &sent_len,
        issendto ? &mbuf->to_addr : NULL);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        mbuf->pos += sent_len;
        return;
    case PAL_SOCKET_ERR_OK: {
        char addr[64];
        pal_socket_addr *_sa = issendto ? &mbuf->to_addr : &o->remote_addr;
        SOCKET_LOG(Debug, o, "Sent message(len=%zu) to %s:%u", mbuf->len,
            pal_socket_addr_get_str_addr(_sa, addr, sizeof(addr)),
            pal_socket_addr_get_port(_sa));
        break;
    }
    default:
        break;
    }

    pal_socket_mbuf_out(o);
    if (!pal_socket_mbuf_top(o)) {
        o->interests.isReadyForWriting = false;
        HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);
    }

    if (mbuf->sent_cb) {
        mbuf->sent_cb(o, err, mbuf->arg);
    }
    pal_mem_free(mbuf);
}

static void pal_socket_handle_recv_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForReading) {
        return;
    }

    pal_socket_obj *o = context;

    if (!o->receiving) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    size_t len = o->recv_maxlen;
    char buf[o->recv_maxlen];
    pal_socket_addr sa;
    pal_socket_err err = pal_socket_recv_async(o, buf, &len,
        o->state == PAL_SOCKET_ST_CONNECTED ? NULL : &sa);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        return;
    case PAL_SOCKET_ERR_OK: {
        char addr[64];
        pal_socket_addr *_sa = o->state == PAL_SOCKET_ST_CONNECTED ? &o->remote_addr : &sa;
        uint16_t port = pal_socket_addr_get_port(_sa);
        pal_socket_addr_get_str_addr(_sa, addr, sizeof(addr));
        o->receiving = false;
        SOCKET_LOG(Debug, o, "Receive message(len=%zu) from %s:%u", len, addr, port);
        if (o->recved_cb) {
            o->recved_cb(o, err, addr, port, buf, len, o->cb_arg);
        }
        break;
    }
    default:
        o->receiving = false;
        if (o->recved_cb) {
            o->recved_cb(o, err, NULL, 0, NULL, 0, o->cb_arg);
        }
        break;
    }
}

static void pal_socket_handle_event_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj *o = context;

    HAPPrecondition(o->handle == fileHandle);

    switch (o->state) {
    case PAL_SOCKET_ST_CONNECTING:
        pal_socket_handle_connect_cb(fileHandle, fileHandleEvents, context);
        break;
    case PAL_SOCKET_ST_ACCEPTING:
        pal_socket_handle_accept_cb(fileHandle, fileHandleEvents, context);
        break;
    case PAL_SOCKET_ST_CONNECTED:
        if (fileHandleEvents.isReadyForWriting) {
            pal_socket_handle_send_cb(fileHandle, fileHandleEvents, context);
        }
        if (fileHandleEvents.isReadyForReading) {
            pal_socket_handle_recv_cb(fileHandle, fileHandleEvents, context);
        }
    default:
        break;
    }
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
        goto err1;
    }

    o->interests.isReadyForReading = true;
    if (HAPPlatformFileHandleRegister(&o->handle, o->fd, o->interests,
        pal_socket_handle_event_cb, o) != kHAPError_None) {
        SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
        goto err1;
    }

    o->type = type;
    o->domain = domain;
    o->id = ++gsocket_count;
    o->mbuf_list_ptail = &o->mbuf_list_head;

    return o;

err1:
    close(o->fd);
err:
    pal_mem_free(o);
    return NULL;
}

void pal_socket_destroy(pal_socket_obj *o) {
    if (!o) {
        return;
    }
    close(o->fd);
    if (o->handle) {
        HAPPlatformFileHandleDeregister(o->handle);
    }
    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
    }
    pal_socket_mbuf *cur;
    while (o->mbuf_list_head) {
        cur = o->mbuf_list_head;
        o->mbuf_list_head = cur->next;
        pal_mem_free(cur);
    }
    pal_mem_free(o);
}

void pal_socket_set_timeout(pal_socket_obj *o, uint32_t ms) {
    HAPPrecondition(o);

    o->timeout = ms;
}

pal_socket_err pal_socket_enable_broadcast(pal_socket_obj *o) {
    HAPPrecondition(o);

    int optval = 1;
    int ret = setsockopt(o->fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (ret != 0) {
        SOCKET_LOG_ERRNO(o, "setsockopt");
        return PAL_SOCKET_ERR_UNKNOWN;
    }
    return PAL_SOCKET_ERR_OK;
}

pal_socket_err pal_socket_bind(pal_socket_obj *o, const char *addr, uint16_t port) {
    HAPPrecondition(o);
    HAPPrecondition(addr);

    int ret;
    pal_socket_addr sa;

    if (!pal_socket_addr_set(&sa, o->domain, addr, port)) {
        return PAL_SOCKET_ERR_INVALID_ARG;
    }

    ret = bind(o->fd, (struct sockaddr *)&sa, pal_socket_addr_set_len(&sa));
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

static void pal_socket_accept_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj *o = context;

    o->timer = 0;
    o->state = PAL_SOCKET_ST_LISTENED;
    if (o->accepted_cb) {
        o->accepted_cb(o, PAL_SOCKET_ERR_TIMEOUT, NULL, NULL, 0, o->cb_arg);
    }
}

pal_socket_err
pal_socket_accept(pal_socket_obj *o, pal_socket_obj **new_o, char *addr, size_t addrlen, uint16_t *port,
    pal_socket_accepted_cb accepted_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(new_o);
    HAPPrecondition(addr);
    HAPPrecondition(addrlen > 0);
    HAPPrecondition(port);
    HAPPrecondition(accepted_cb);

    if (o->state != PAL_SOCKET_ST_LISTENED) {
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    pal_socket_addr sa;
    pal_socket_err err = pal_socket_accept_async(o, new_o, &sa);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
            HAPPlatformClockGetCurrent() + o->timeout,
            pal_socket_accept_timeout_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "Failed to create timeout timer.");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
        o->state = PAL_SOCKET_ST_ACCEPTING;
        o->accepted_cb = accepted_cb;
        o->cb_arg = arg;
        SOCKET_LOG(Debug, o, "Accepting ...");
        break;
    case PAL_SOCKET_ERR_OK: {
        *port = pal_socket_addr_get_port(&sa);
        pal_socket_addr_get_str_addr(&sa, addr, addrlen);
        SOCKET_LOG(Debug, o, "Accept a connection from %s:%d", addr, *port);
        break;
    }
    default:
        break;
    }

    return err;
}

static void pal_socket_connect_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj *o = context;

    o->timer = 0;
    o->state = PAL_SOCKET_ST_NONE;
    o->interests.isReadyForWriting = false;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);

    if (o->connected_cb) {
        o->connected_cb(o, PAL_SOCKET_ERR_TIMEOUT, o->cb_arg);
    }
}

pal_socket_err pal_socket_connect(pal_socket_obj *o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(addr);
    HAPPrecondition(connected_cb);

    if (o->state != PAL_SOCKET_ST_NONE) {
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    if (!pal_socket_addr_set(&o->remote_addr, o->domain, addr, port)) {
        return PAL_SOCKET_ERR_INVALID_ARG;
    }

    pal_socket_err err = pal_socket_connect_async(o);
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS:
        if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
            HAPPlatformClockGetCurrent() + o->timeout,
            pal_socket_connect_timeout_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "Failed to create timeout timer.");
            return PAL_SOCKET_ERR_UNKNOWN;
        }
        o->interests.isReadyForWriting = true;
        HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);
        o->state = PAL_SOCKET_ST_CONNECTING;
        o->connected_cb = connected_cb;
        o->cb_arg = arg;
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

pal_socket_err pal_socket_send(pal_socket_obj *o, const void *data, size_t len, pal_socket_sent_cb sent_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(sent_cb);
    if (len > 0) {
        HAPPrecondition(data);
    }

    if (o->state != PAL_SOCKET_ST_CONNECTED) {
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    size_t sent_len;
    pal_socket_err err;
    if (pal_socket_mbuf_top(o)) {
        sent_len = 0;
        err = PAL_SOCKET_ERR_IN_PROGRESS;
    } else {
        sent_len = len;
        err = pal_socket_send_async(o, data, &sent_len, NULL);
    }
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS: {
        pal_socket_mbuf *mbuf = pal_socket_mbuf_create(data + sent_len, len - sent_len, NULL, sent_cb, arg);
        if (!mbuf) {
            return PAL_SOCKET_ERR_ALLOC;
        }
        pal_socket_mbuf_in(o, mbuf);
        o->interests.isReadyForWriting = true;
        HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);
        SOCKET_LOG(Debug, o, "Sending packet(len=%zu) ...", len);
        break;
    }
    case PAL_SOCKET_ERR_OK: {
        char addr[64];
        SOCKET_LOG(Debug, o, "Sent packet(len=%zd) to %s:%u", len,
            pal_socket_addr_get_str_addr(&o->remote_addr, addr, sizeof(addr)),
            pal_socket_addr_get_port(&o->remote_addr));
        break;
    }
    default:
        break;
    }

    return err;
}

pal_socket_err pal_socket_sendto(pal_socket_obj *o, const void *data, size_t len,
    const char *addr, uint16_t port, pal_socket_sent_cb sent_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(addr);
    HAPPrecondition(sent_cb);
    if (len > 0) {
        HAPPrecondition(data);
    }

    if (o->state != PAL_SOCKET_ST_CONNECTED) {
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    pal_socket_addr sa;
    if (!pal_socket_addr_set(&sa, o->domain, addr, port)) {
        return PAL_SOCKET_ERR_INVALID_ARG;
    }

    size_t sent_len;
    pal_socket_err err;
    if (pal_socket_mbuf_top(o)) {
        sent_len = 0;
        err = PAL_SOCKET_ERR_IN_PROGRESS;
    } else {
        sent_len = len;
        err = pal_socket_send_async(o, data, &sent_len, &sa);
    }
    switch (err) {
    case PAL_SOCKET_ERR_IN_PROGRESS: {
        pal_socket_mbuf *mbuf = pal_socket_mbuf_create(data + sent_len, len - sent_len, &sa, sent_cb, arg);
        if (!mbuf) {
            return PAL_SOCKET_ERR_ALLOC;
        }
        pal_socket_mbuf_in(o, mbuf);
        o->interests.isReadyForWriting = true;
        HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);
        SOCKET_LOG(Debug, o, "Sending message(len=%zu) ...", len);
        break;
    }
    case PAL_SOCKET_ERR_OK:
        SOCKET_LOG(Debug, o, "Sent message(len=%zu) to %s:%u", len, addr, port);
        break;
    default:
        break;
    }

    return err;
}

static void pal_socket_recv_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj *o = context;

    o->timer = 0;
    o->receiving = false;
    o->interests.isReadyForWriting = false;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, pal_socket_handle_event_cb, o);

    if (o->recved_cb) {
        o->recved_cb(o, PAL_SOCKET_ERR_TIMEOUT, NULL, 0, NULL, 0, o->cb_arg);
    }
}

pal_socket_err pal_socket_recv(pal_socket_obj *o, size_t maxlen, pal_socket_recved_cb recved_cb, void *arg) {
    HAPPrecondition(o);
    HAPPrecondition(maxlen > 0);
    HAPPrecondition(recved_cb);

    if (o->type == PAL_SOCKET_TYPE_TCP && o->state != PAL_SOCKET_ST_CONNECTED) {
        return PAL_SOCKET_ERR_INVALID_STATE;
    }

    if (o->receiving) {
        return PAL_SOCKET_ERR_BUSY;
    }

    if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
        HAPPlatformClockGetCurrent() + o->timeout,
        pal_socket_recv_timeout_cb, o) != kHAPError_None) {
        SOCKET_LOG(Error, o, "Failed to create timeout timer.");
        return PAL_SOCKET_ERR_UNKNOWN;
    }

    o->recv_maxlen = maxlen;
    o->recved_cb = recved_cb;
    o->cb_arg = arg;
    o->receiving = true;

    SOCKET_LOG(Debug, o, "Receiving ...");

    return PAL_SOCKET_ERR_IN_PROGRESS;
}

const char *pal_socket_get_error_str(pal_socket_err err) {
    HAPPrecondition(err >= PAL_SOCKET_ERR_OK && err < PAL_SOCKET_ERR_COUNT);
    const char *err_strs[] = {
        [PAL_SOCKET_ERR_OK] = "no error",
        [PAL_SOCKET_ERR_TIMEOUT] = "timeout",
        [PAL_SOCKET_ERR_IN_PROGRESS] = "the opreation is in progress",
        [PAL_SOCKET_ERR_UNKNOWN] = "unknown error",
        [PAL_SOCKET_ERR_ALLOC] = "failed to alloc",
        [PAL_SOCKET_ERR_INVALID_ARG] = "invalid argument",
        [PAL_SOCKET_ERR_INVALID_STATE] = "invalid state",
        [PAL_SOCKET_ERR_CLOSED] = "the peer closed the connection",
        [PAL_SOCKET_ERR_BUSY] = "busy now, try again later",
    };
    return err_strs[err];
}
