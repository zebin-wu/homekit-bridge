// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pal/socket.h>
#include <pal/mem.h>

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

#define PAL_SOCKET_OBJ_MAGIC 0x1515

HAP_ENUM_BEGIN(uint8_t, pal_socket_state) {
    PAL_SOCKET_ST_NONE,
    PAL_SOCKET_ST_CONNECTING,
    PAL_SOCKET_ST_CONNECTED,
    PAL_SOCKET_ST_HANDSHAKING,
    PAL_SOCKET_ST_HANDSHAKED,
    PAL_SOCKET_ST_LISTENED,
    PAL_SOCKET_ST_ACCEPTING
} HAP_ENUM_END(uint8_t, pal_socket_state);

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
    bool all;
    char buf[0];
} pal_socket_mbuf;

typedef struct pal_socket_obj_int {
    uint16_t magic;
    bool receiving;
    pal_socket_state state;

    pal_socket_type type;
    pal_net_addr_family af;
    uint16_t id;
    int fd;
    uint32_t timeout;
    HAPPlatformTimerRef timer;
    void *recv_buf;
    size_t recv_buflen;

    pal_socket_addr remote_addr;

    void *cb;
    void *cb_arg;

    struct pal_socket_obj_int *accept_new_obj;

    HAPPlatformFileHandleCallback handle_cb;
    HAPPlatformFileHandleRef handle;
    HAPPlatformFileHandleEvent interests;

    pal_socket_mbuf *mbuf_list_head;
    pal_socket_mbuf **mbuf_list_ptail;

    void *bio_ctx;
    pal_socket_bio_method bio_method;
} pal_socket_obj_int;
HAP_STATIC_ASSERT(sizeof(pal_socket_obj) >= sizeof(pal_socket_obj_int), pal_socket_obj_int);

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
pal_socket_addr_set(pal_socket_addr *addr, pal_net_addr_family af, const char *str_addr, uint16_t port) {
    switch (af) {
    case PAL_NET_ADDR_FAMILY_INET: {
        struct sockaddr_in *sa = &addr->in;
        sa->sin_family = AF_INET;
        int ret = inet_pton(AF_INET, str_addr, &sa->sin_addr);
        if (ret <= 0) {
            return false;
        }
        sa->sin_port = htons(port);
        break;
    }
    case PAL_NET_ADDR_FAMILY_INET6: {
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
        HAPFatalError();
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
        HAPFatalError();
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
        HAPFatalError();
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
        HAPFatalError();
    }
    return NULL;
}

static pal_socket_mbuf *pal_socket_mbuf_create(const void *data, size_t len,
    pal_socket_addr *to_addr, bool all, pal_socket_sent_cb sent_cb, void *arg) {
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
    mbuf->all = all;
    mbuf->sent_cb = sent_cb;
    mbuf->arg = arg;

    return mbuf;
}

static void pal_socket_mbuf_in(pal_socket_obj_int *o, pal_socket_mbuf *mbuf) {
    mbuf->next = NULL;
    *(o->mbuf_list_ptail) = mbuf;
    o->mbuf_list_ptail = &mbuf->next;
}

static pal_socket_mbuf *pal_socket_mbuf_top(pal_socket_obj_int *o) {
    return o->mbuf_list_head;
}

static pal_socket_mbuf *pal_socket_mbuf_out(pal_socket_obj_int *o) {
    pal_socket_mbuf *mbuf = o->mbuf_list_head;
    if (mbuf) {
        o->mbuf_list_head = mbuf->next;
        if (o->mbuf_list_head == NULL) {
            o->mbuf_list_ptail = &o->mbuf_list_head;
        }
    }
    return mbuf;
}

static bool pal_socket_set_nonblock(pal_socket_obj_int *o) {
    if (fcntl(o->fd, F_SETFL, fcntl(o->fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        SOCKET_LOG_ERRNO(o, "fcntl");
        return false;
    }
    return true;
}

static bool pal_socket_connected(pal_socket_obj_int *o) {
    return o->bio_ctx ? o->state == PAL_SOCKET_ST_HANDSHAKED :
        o->state == PAL_SOCKET_ST_CONNECTED;
}

static void pal_socket_update_interests(pal_socket_obj_int *o, bool read, bool write) {
    o->interests.isReadyForReading = read;
    o->interests.isReadyForWriting = write;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, o->handle_cb, o);
}

static void pal_socket_enable_read(pal_socket_obj_int *o, bool flag) {
    o->interests.isReadyForReading = flag;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, o->handle_cb, o);
}

static void pal_socket_enable_write(pal_socket_obj_int *o, bool flag) {
    o->interests.isReadyForWriting = flag;
    HAPPlatformFileHandleUpdateInterests(o->handle, o->interests, o->handle_cb, o);
}

static void pal_socket_recv_reset(pal_socket_obj_int *o) {
    o->recv_buf = NULL;
    o->recv_buflen = 0;
    o->receiving = false;
    pal_socket_enable_read(o, false);
}

static pal_err pal_socket_accept_async(pal_socket_obj_int *o, pal_socket_obj_int *new_o, pal_socket_addr *addr) {
    HAPPrecondition(o);
    HAPPrecondition(new_o);
    HAPPrecondition(addr);

    int new_fd;
    socklen_t addrlen = sizeof(*addr);

    do {
        new_fd = accept(o->fd, (struct sockaddr *)addr, &addrlen);
    } while (new_fd == -1 && errno == EINTR);
    if (new_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return PAL_ERR_IN_PROGRESS;
        } else {
            SOCKET_LOG_ERRNO(o, "accept");
            return PAL_ERR_UNKNOWN;
        }
    }

    memset(new_o, 0, sizeof(*new_o));
    new_o->fd = new_fd;
    new_o->type = o->type;
    new_o->af = o->af;
    new_o->id = ++gsocket_count;
    new_o->state = PAL_SOCKET_ST_CONNECTED;
    new_o->remote_addr = *addr;
    new_o->handle_cb = o->handle_cb;

    if (!pal_socket_set_nonblock(new_o)) {
        SOCKET_LOG(Error, new_o, "%s: Failed to set non-block.", __func__);
        goto err;
    }

    if (HAPPlatformFileHandleRegister(&new_o->handle, new_fd, new_o->interests,
        new_o->handle_cb, new_o) != kHAPError_None) {
        SOCKET_LOG(Error, new_o, "%s: Failed to register handle callback.", __func__);
        goto err;
    }

    new_o->magic = PAL_SOCKET_OBJ_MAGIC;
    return PAL_ERR_OK;

err:
    close(new_fd);
    memset(new_o, 0, sizeof(*new_o));
    return PAL_ERR_UNKNOWN;
}

static pal_err pal_socket_connect_async(pal_socket_obj_int *o) {
    int ret;

    do {
        ret = connect(o->fd, (struct sockaddr *)&o->remote_addr,
            pal_socket_addr_get_len(&o->remote_addr));
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        switch (errno) {
        case EINPROGRESS:
            return PAL_ERR_IN_PROGRESS;
        case EISCONN:
            return PAL_ERR_OK;
        default:
            SOCKET_LOG_ERRNO(o, "connect");
            return PAL_ERR_UNKNOWN;
        }
    }

    return PAL_ERR_OK;
}

static pal_err
pal_socket_raw_sendto(pal_socket_obj_int *o, const void *data, size_t *len, pal_socket_addr *addr) {
    ssize_t rc;
    socklen_t addrlen = addr ? pal_socket_addr_get_len(addr) : 0;

    do {
        rc = sendto(o->fd, data, *len, 0, (struct sockaddr *)addr, addrlen);
    } while (rc == -1 && errno == EINTR);
    if (rc == -1) {
        *len = 0;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return PAL_ERR_AGAIN;
        } else {
            SOCKET_LOG_ERRNO(o, "sendto");
            return PAL_ERR_UNKNOWN;
        }
    }
    *len = rc;
    return PAL_ERR_OK;
}

static pal_err
pal_socket_sendto_async(pal_socket_obj_int *o, const void *data, size_t *len, pal_socket_addr *addr) {
    if (o->bio_ctx) {
        if (addr) {
            SOCKET_LOG(Error, o, "BIO not support 'sendto'");
            return PAL_ERR_UNKNOWN;
        }
        return o->bio_method.send(o->bio_ctx, data, len);
    }

    return pal_socket_raw_sendto(o, data, len, addr);
}

static pal_err
pal_socket_raw_recvfrom(pal_socket_obj_int *o, void *buf, size_t *len, pal_socket_addr *addr) {
    ssize_t rc;
    socklen_t addrlen = sizeof(*addr);

    do {
        rc = recvfrom(o->fd, buf, *len, 0, (struct sockaddr *)addr, &addrlen);
    } while (rc == -1 && errno == EINTR);
    if (rc == -1) {
        *len = 0;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return PAL_ERR_AGAIN;
        } else {
            SOCKET_LOG_ERRNO(o, "recvfrom");
            return PAL_ERR_UNKNOWN;
        }
    }
    *len = rc;
    return PAL_ERR_OK;
}

static pal_err
pal_socket_recvfrom_async(pal_socket_obj_int *o, void *buf, size_t *len, pal_socket_addr *addr) {
    if (o->bio_ctx) {
        if (addr) {
            SOCKET_LOG(Error, o, "BIO not support 'recvfrom'");
            return PAL_ERR_UNKNOWN;
        }
        return o->bio_method.recv(o->bio_ctx, buf, len);
    }

    return pal_socket_raw_recvfrom(o, buf, len, addr);
}

static void pal_socket_handle_accept_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForReading) {
        return;
    }

    pal_socket_obj_int *o = context;

    if (o->state != PAL_SOCKET_ST_ACCEPTING) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    char buf[64];
    uint16_t port = 0;
    pal_socket_addr sa;
    pal_socket_obj_int *new_o = o->accept_new_obj;
    const char *addr = NULL;

    pal_err err = pal_socket_accept_async(o, new_o, &sa);
    switch (err) {
    case PAL_ERR_IN_PROGRESS:
        return;
    case PAL_ERR_OK: {
        port = pal_socket_addr_get_port(&new_o->remote_addr);
        addr = pal_socket_addr_get_str_addr(&new_o->remote_addr, buf, sizeof(buf));
        SOCKET_LOG(Debug, o, "Accept a connection(TCP:%u) from %s:%d", new_o->id, addr, port);
        break;
    }
    default:
        break;
    }

    o->state = PAL_SOCKET_ST_LISTENED;
    pal_socket_enable_read(o, false);

    HAPAssert(o->cb);
    pal_socket_accepted_cb cb = o->cb;
    o->accept_new_obj = NULL;
    o->cb = NULL;
    cb((pal_socket_obj *)o, err, addr, port, o->cb_arg);
}

static void pal_socket_handle_connect_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForWriting) {
        return;
    }

    pal_socket_obj_int *o = context;

    if (o->state != PAL_SOCKET_ST_CONNECTING) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    pal_err err = pal_socket_connect_async(o);
    switch (err) {
    case PAL_ERR_IN_PROGRESS:
        return;
    case PAL_ERR_OK: {
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

    pal_socket_enable_write(o, false);

    HAPAssert(o->cb);
    pal_socket_connected_cb cb = o->cb;
    o->cb = NULL;
    cb((pal_socket_obj *)o, err, o->cb_arg);
}

static void pal_socket_handle_handshake_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj_int *o = context;
    HAPAssert(o->bio_ctx);

    if (o->state != PAL_SOCKET_ST_HANDSHAKING) {
        return;
    }

    pal_err err = o->bio_method.handshake(o->bio_ctx);
    switch (err) {
    case PAL_ERR_OK:
        o->state = PAL_SOCKET_ST_HANDSHAKED;
        SOCKET_LOG(Debug, o, "Handshaked");
        break;
    case PAL_ERR_WANT_READ:
        pal_socket_update_interests(o, true, false);
        return;
    case PAL_ERR_WANT_WRITE:
        pal_socket_update_interests(o, false, true);
        return;
    default:
        o->state = PAL_SOCKET_ST_CONNECTED;
        SOCKET_LOG(Error, o, "Handshake failed.");
        break;
    }

    pal_socket_update_interests(o, false, false);

    HAPAssert(o->cb);
    pal_socket_handshaked_cb cb = o->cb;
    o->cb = NULL;
    cb((pal_socket_obj *)o, err, o->cb_arg);
}

static void pal_socket_handle_send_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    if (!fileHandleEvents.isReadyForWriting) {
        return;
    }

    pal_socket_obj_int *o = context;
    pal_socket_mbuf *mbuf = pal_socket_mbuf_top(o);
    if (!mbuf) {
        return;
    }

    bool issendto = mbuf->to_addr.in.sin_family != AF_UNSPEC;
    size_t sent_len = mbuf->len - mbuf->pos;
    pal_err err = pal_socket_sendto_async(o, mbuf->buf + mbuf->pos, &sent_len,
        issendto ? &mbuf->to_addr : NULL);
    switch (err) {
    case PAL_ERR_OK: {
        char addr[64];
        pal_socket_addr *_sa = issendto ? &mbuf->to_addr : &o->remote_addr;
        if (sent_len == mbuf->len) {
            SOCKET_LOG(Debug, o, "Sent message(len=%zu) to %s:%u", mbuf->len,
                pal_socket_addr_get_str_addr(_sa, addr, sizeof(addr)),
                pal_socket_addr_get_port(_sa));
        } else if (mbuf->all) {
            mbuf->pos += sent_len;
            return;
        } else {
            SOCKET_LOG(Debug, o, "Only sent %zu bytes message(len=%zu) to %s:%u",
                sent_len, mbuf->len,
                pal_socket_addr_get_str_addr(_sa, addr, sizeof(addr)),
                pal_socket_addr_get_port(_sa));
        }
        break;
    }
    case PAL_ERR_AGAIN:
        HAPFatalError();
    default:
        break;
    }

    pal_socket_mbuf_out(o);
    if (!pal_socket_mbuf_top(o)) {
        pal_socket_enable_write(o, false);
    }

    if (mbuf->sent_cb) {
        mbuf->sent_cb((pal_socket_obj *)o, err, sent_len, mbuf->arg);
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

    pal_socket_obj_int *o = context;

    if (!o->receiving) {
        return;
    }

    if (o->timer) {
        HAPPlatformTimerDeregister(o->timer);
        o->timer = 0;
    }

    uint16_t port = 0;
    const char *addr = NULL;
    size_t len = o->recv_buflen;
    char addrbuf[64];
    pal_socket_addr sa;
    pal_err err = pal_socket_recvfrom_async(o, o->recv_buf, &len,
        pal_socket_connected(o) ? NULL : &sa);
    switch (err) {
    case PAL_ERR_AGAIN:
        return;
    case PAL_ERR_OK: {
        pal_socket_addr *_sa = pal_socket_connected(o) ? &o->remote_addr : &sa;
        port = pal_socket_addr_get_port(_sa);
        addr = pal_socket_addr_get_str_addr(_sa, addrbuf, sizeof(addrbuf));
        SOCKET_LOG(Debug, o, "Received message(len=%zu) from %s:%u", len, addr, port);
        break;
    }
    default:
        break;
    }

    pal_socket_recv_reset(o);

    HAPAssert(o->cb);
    pal_socket_recved_cb cb = o->cb;
    o->cb = NULL;
    cb((pal_socket_obj *)o, err, addr, port, len, o->cb_arg);
}

static void pal_socket_tcp_handle_event_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj_int *o = context;

    HAPPrecondition(o->handle == fileHandle);

    switch (o->state) {
    case PAL_SOCKET_ST_CONNECTING:
        pal_socket_handle_connect_cb(fileHandle, fileHandleEvents, context);
        break;
    case PAL_SOCKET_ST_HANDSHAKING:
        pal_socket_handle_handshake_cb(fileHandle, fileHandleEvents, context);
        break;
    case PAL_SOCKET_ST_ACCEPTING:
        pal_socket_handle_accept_cb(fileHandle, fileHandleEvents, context);
        break;
    case PAL_SOCKET_ST_CONNECTED:
    case PAL_SOCKET_ST_HANDSHAKED:
        if (fileHandleEvents.isReadyForReading) {
            pal_socket_handle_recv_cb(fileHandle, fileHandleEvents, context);
        } else if (fileHandleEvents.isReadyForWriting) {
            pal_socket_handle_send_cb(fileHandle, fileHandleEvents, context);
        }
        break;
    default:
        break;
    }
}

static void pal_socket_udp_handle_event_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void *context) {
    pal_socket_obj_int *o = context;

    HAPPrecondition(o->handle == fileHandle);

    if (fileHandleEvents.isReadyForReading) {
        pal_socket_handle_recv_cb(fileHandle, fileHandleEvents, context);
    } else if (fileHandleEvents.isReadyForWriting) {
        pal_socket_handle_send_cb(fileHandle, fileHandleEvents, context);
    }
}

bool pal_socket_obj_init(pal_socket_obj *_o, pal_socket_type type, pal_net_addr_family af) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    int _af, _type, _protocol;

    memset(o, 0, sizeof(*o));

    switch (af) {
    case PAL_NET_ADDR_FAMILY_INET:
        _af = AF_INET;
        break;
    case PAL_NET_ADDR_FAMILY_INET6:
        _af = AF_INET6;
        break;
    default:
        HAPFatalError();
    }
    switch (type) {
    case PAL_SOCKET_TYPE_TCP:
        _type = SOCK_STREAM;
        _protocol = IPPROTO_TCP;
        o->handle_cb = pal_socket_tcp_handle_event_cb;
        break;
    case PAL_SOCKET_TYPE_UDP:
        _type = SOCK_DGRAM;
        _protocol = IPPROTO_UDP;
        o->handle_cb = pal_socket_udp_handle_event_cb;
        break;
    default:
        HAPFatalError();
    }

    o->fd = socket(_af, _type, _protocol);
    if (o->fd == -1) {
        SOCKET_LOG_ERRNO(o, "socket");
        goto err;
    }

    if (!pal_socket_set_nonblock(o)) {
        goto err1;
    }

    if (HAPPlatformFileHandleRegister(&o->handle, o->fd, o->interests,
        o->handle_cb, o) != kHAPError_None) {
        SOCKET_LOG(Error, o, "%s: Failed to register handle callback", __func__);
        goto err1;
    }

    o->type = type;
    o->af = af;
    o->id = ++gsocket_count;
    o->mbuf_list_ptail = &o->mbuf_list_head;

    o->magic = PAL_SOCKET_OBJ_MAGIC;
    SOCKET_LOG(Debug, o, "Initialized.");
    return true;

err1:
    close(o->fd);
err:
    memset(o, 0, sizeof(*o));
    return false;
}

void pal_socket_obj_deinit(pal_socket_obj *_o) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    SOCKET_LOG(Debug, o, "%s(%p)", __func__, o);

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
    memset(o, 0, sizeof(*o));
}

void pal_socket_set_timeout(pal_socket_obj *_o, uint32_t ms) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    o->timeout = ms;
}

pal_err pal_socket_enable_broadcast(pal_socket_obj *_o) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    int optval = 1;
    int ret = setsockopt(o->fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (ret != 0) {
        SOCKET_LOG_ERRNO(o, "setsockopt");
        return PAL_ERR_UNKNOWN;
    }
    return PAL_ERR_OK;
}

pal_err pal_socket_bind(pal_socket_obj *_o, const char *addr, uint16_t port) {
    HAPPrecondition(_o);
    HAPPrecondition(addr);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    int ret;
    pal_socket_addr sa;

    SOCKET_LOG(Debug, o, "%s(addr = \"%s\", port = %u)", __func__, addr, port);

    if (!pal_socket_addr_set(&sa, o->af, addr, port)) {
        return PAL_ERR_INVALID_ARG;
    }

    ret = bind(o->fd, (struct sockaddr *)&sa, pal_socket_addr_get_len(&sa));
    if (ret == -1) {
        SOCKET_LOG_ERRNO(o, "bind");
        return PAL_ERR_UNKNOWN;
    }
    SOCKET_LOG(Debug, o, "Bound to %s:%u", addr, port);
    return PAL_ERR_OK;
}

pal_err pal_socket_listen(pal_socket_obj *_o, int backlog) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    SOCKET_LOG(Debug, o, "%s(backlog = %d)", __func__, backlog);

    if (o->state != PAL_SOCKET_ST_NONE) {
        return PAL_ERR_INVALID_STATE;
    }

    int ret = listen(o->fd, backlog);
    if (ret == -1) {
        SOCKET_LOG_ERRNO(o, "listen");
        return PAL_ERR_UNKNOWN;
    }
    o->state = PAL_SOCKET_ST_LISTENED;
    return PAL_ERR_OK;
}

static void pal_socket_accept_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj_int *o = context;

    o->timer = 0;
    o->state = PAL_SOCKET_ST_LISTENED;
    pal_socket_enable_read(o, false);

    HAPAssert(o->cb);
    pal_socket_accepted_cb cb = o->cb;
    o->accept_new_obj = NULL;
    o->cb = NULL;
    cb((pal_socket_obj *)o, PAL_ERR_TIMEOUT, NULL, 0, o->cb_arg);
}

pal_err
pal_socket_accept(pal_socket_obj *_o, pal_socket_obj *_new_o, char *addr, size_t addrlen, uint16_t *port,
    pal_socket_accepted_cb accepted_cb, void *arg) {
    HAPPrecondition(_o);
    HAPPrecondition(_new_o);
    HAPPrecondition(addr);
    HAPPrecondition(addrlen > 0);
    HAPPrecondition(port);
    HAPPrecondition(accepted_cb);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    pal_socket_obj_int *new_o = (pal_socket_obj_int *)_new_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    SOCKET_LOG(Debug, o, "%s()", __func__);

    if (o->state != PAL_SOCKET_ST_LISTENED) {
        return PAL_ERR_INVALID_STATE;
    }

    pal_socket_addr sa;
    pal_err err = pal_socket_accept_async(o, new_o, &sa);
    switch (err) {
    case PAL_ERR_IN_PROGRESS:
        if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
            HAPPlatformClockGetCurrent() + o->timeout,
            pal_socket_accept_timeout_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "Failed to create timeout timer.");
            return PAL_ERR_UNKNOWN;
        }
        o->accept_new_obj = new_o;
        o->state = PAL_SOCKET_ST_ACCEPTING;
        o->cb = accepted_cb;
        o->cb_arg = arg;
        pal_socket_enable_read(o, true);
        SOCKET_LOG(Debug, o, "Accepting ...");
        break;
    case PAL_ERR_OK: {
        *port = pal_socket_addr_get_port(&sa);
        pal_socket_addr_get_str_addr(&sa, addr, addrlen);
        SOCKET_LOG(Debug, o, "Accept a connection(TCP:%u) from %s:%d", new_o->id, addr, *port);
        break;
    }
    default:
        break;
    }

    return err;
}

static void pal_socket_connect_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj_int *o = context;

    o->timer = 0;
    o->state = PAL_SOCKET_ST_NONE;
    pal_socket_enable_write(o, false);

    HAPAssert(o->cb);
    pal_socket_connected_cb cb = o->cb;
    o->cb = NULL;
    cb((pal_socket_obj *)o, PAL_ERR_TIMEOUT, o->cb_arg);
}

pal_err pal_socket_connect(pal_socket_obj *_o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg) {
    HAPPrecondition(_o);
    HAPPrecondition(addr);
    HAPPrecondition(connected_cb);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    SOCKET_LOG(Debug, o, "%s(addr = \"%s\", port = %u)", __func__, addr, port);

    if (o->state != PAL_SOCKET_ST_NONE) {
        return PAL_ERR_INVALID_STATE;
    }

    if (!pal_socket_addr_set(&o->remote_addr, o->af, addr, port)) {
        return PAL_ERR_INVALID_ARG;
    }

    pal_err err = pal_socket_connect_async(o);
    switch (err) {
    case PAL_ERR_IN_PROGRESS:
        if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
            HAPPlatformClockGetCurrent() + o->timeout,
            pal_socket_connect_timeout_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "Failed to create timeout timer.");
            return PAL_ERR_UNKNOWN;
        }
        pal_socket_enable_write(o, true);
        o->state = PAL_SOCKET_ST_CONNECTING;
        o->cb = connected_cb;
        o->cb_arg = arg;
        SOCKET_LOG(Debug, o, "Connecting to %s:%u ...", addr, port);
        break;
    case PAL_ERR_OK:
        o->state = PAL_SOCKET_ST_CONNECTED;
        SOCKET_LOG(Debug, o, "Connected to %s:%u", addr, port);
        break;
    default:
        break;
    }

    return err;
}

pal_err pal_socket_send(pal_socket_obj *o, const void *data,
    size_t *len, bool all, pal_socket_sent_cb sent_cb, void *arg) {
    return pal_socket_sendto(o, data, len, NULL, 0, all, sent_cb, arg);
}

pal_err pal_socket_sendto(pal_socket_obj *_o, const void *data, size_t *len,
    const char *addr, uint16_t port, bool all, pal_socket_sent_cb sent_cb, void *arg) {
    HAPPrecondition(_o);
    HAPPrecondition(sent_cb);
    HAPPrecondition(len);
    if (*len > 0) {
        HAPPrecondition(data);
    }

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    if (addr) {
        SOCKET_LOG(Debug, o, "sendto(len = %zu, addr = \"%s\", port = %u)", *len, addr, port);
    } else {
        SOCKET_LOG(Debug, o, "send(len = %zu)", *len);
    }

    if (o->type == PAL_SOCKET_TYPE_TCP && !pal_socket_connected(o)) {
        return PAL_ERR_INVALID_STATE;
    }

    char buf[64];
    pal_socket_addr *psa = NULL;
    pal_socket_addr sa;
    if (addr) {
        psa = &sa;
        if (!pal_socket_addr_set(psa, o->af, addr, port)) {
            return PAL_ERR_INVALID_ARG;
        }
    } else {
        addr = pal_socket_addr_get_str_addr(&o->remote_addr, buf, sizeof(buf));
        port = pal_socket_addr_get_port(&o->remote_addr);
    }

    size_t sent_len;
    pal_err err;
    if (pal_socket_mbuf_top(o)) {
        sent_len = 0;
        err = PAL_ERR_IN_PROGRESS;
    } else {
        sent_len = *len;
        err = pal_socket_sendto_async(o, data, &sent_len, psa);
    }
    switch (err) {
    case PAL_ERR_AGAIN: {
        pal_socket_mbuf *mbuf = pal_socket_mbuf_create(data, *len, psa, all, sent_cb, arg);
        if (!mbuf) {
            return PAL_ERR_ALLOC;
        }
        err = PAL_ERR_IN_PROGRESS;
        pal_socket_mbuf_in(o, mbuf);
        pal_socket_enable_write(o, true);
        SOCKET_LOG(Debug, o, "Sending message(len=%zu) to %s:%u ...", *len, addr, port);
        break;
    }
    case PAL_ERR_OK:
        if (sent_len == *len) {
            SOCKET_LOG(Debug, o, "Sent message(len=%zu) to %s:%u", *len, addr, port);
        } else if (all) {
            pal_socket_mbuf *mbuf = pal_socket_mbuf_create(data + sent_len, *len - sent_len,
                psa, all, sent_cb, arg);
            if (!mbuf) {
                return PAL_ERR_ALLOC;
            }
            pal_socket_mbuf_in(o, mbuf);
            pal_socket_enable_write(o, true);
            SOCKET_LOG(Debug, o, "Sending message(len=%zu) to %s:%u ...", *len, addr, port);
        } else {
            SOCKET_LOG(Debug, o, "Only sent %zu bytes message(len=%zu) to %s:%u",
                sent_len, *len, addr, port);
        }
        break;
    default:
        break;
    }

    *len = sent_len;

    return err;
}

static void pal_socket_recv_timeout_cb(HAPPlatformTimerRef timer, void *context) {
    pal_socket_obj_int *o = context;

    o->timer = 0;
    pal_socket_recv_reset(o);

    HAPAssert(o->cb);
    pal_socket_recved_cb cb = o->cb;
    o->cb = NULL;
    cb((pal_socket_obj *)o, PAL_ERR_TIMEOUT, NULL, 0, 0, o->cb_arg);
}

pal_err pal_socket_recv(pal_socket_obj *o, void *buf, size_t *len,
    pal_socket_recved_cb recved_cb, void *arg) {
    return pal_socket_recvfrom(o, buf, len, NULL, 0, NULL, recved_cb, arg);
}

pal_err pal_socket_recvfrom(pal_socket_obj *_o, void *buf, size_t *len, char *addr,
    size_t addrlen, uint16_t *port, pal_socket_recved_cb recved_cb, void *arg) {
    HAPPrecondition(_o);
    HAPPrecondition(buf);
    HAPPrecondition(len);
    HAPPrecondition(*len > 0);
    HAPPrecondition(recved_cb);
    if (addr) {
        HAPPrecondition(addrlen > 0);
        HAPPrecondition(port);
    }

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    SOCKET_LOG(Debug, o, "%s(len = %zu)", addr ? __func__ : "pal_socket_recv", *len);

    if (o->type == PAL_SOCKET_TYPE_TCP && !pal_socket_connected(o)) {
        return PAL_ERR_INVALID_STATE;
    }

    if (o->receiving) {
        return PAL_ERR_BUSY;
    }

    pal_socket_addr sa;
    size_t recvlen = *len;
    pal_err err = pal_socket_recvfrom_async(o, buf, &recvlen,
        pal_socket_connected(o) ? NULL : &sa);
    switch (err) {
    case PAL_ERR_AGAIN:
        if (o->timeout != 0 && HAPPlatformTimerRegister(&o->timer,
            HAPPlatformClockGetCurrent() + o->timeout,
            pal_socket_recv_timeout_cb, o) != kHAPError_None) {
            SOCKET_LOG(Error, o, "Failed to create timeout timer.");
            return PAL_ERR_UNKNOWN;
        }
        err = PAL_ERR_IN_PROGRESS;
        o->recv_buf = buf;
        o->recv_buflen = *len;
        o->cb = recved_cb;
        o->cb_arg = arg;
        o->receiving = true;
        pal_socket_enable_read(o, true);
        SOCKET_LOG(Debug, o, "Receiving ...");
        break;
    case PAL_ERR_OK: {
        *len = recvlen;
        if (addr) {
            pal_socket_addr *_sa = pal_socket_connected(o) ? &o->remote_addr : &sa;
            *port = pal_socket_addr_get_port(_sa);
            pal_socket_addr_get_str_addr(_sa, addr, addrlen);
            SOCKET_LOG(Debug, o, "Received message(len=%zu) from %s:%u", *len, addr, *port);
        } else {
            SOCKET_LOG(Debug, o, "Received message(len=%zu)", *len);
        }
        break;
    }
    default:
        break;
    }

    return err;
}

bool pal_socket_readable(pal_socket_obj *_o) {
    HAPPrecondition(_o);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    if (o->bio_ctx && o->bio_method.pending(o->bio_ctx)) {
        return true;
    }

    fd_set read_fds;
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };
    FD_ZERO(&read_fds);
    FD_SET(o->fd, &read_fds);
    return select(o->fd + 1, &read_fds, NULL, NULL, &tv) == 1 && FD_ISSET(o->fd, &read_fds);
}

pal_err pal_socket_handshake(pal_socket_obj *_o,
    pal_socket_handshaked_cb handshaked_cb, void *arg) {
    HAPPrecondition(_o);
    HAPPrecondition(handshaked_cb);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    if (!o->bio_ctx) {
        SOCKET_LOG(Error, o, "Initialize BIO first.");
        return PAL_ERR_INVALID_STATE;
    }

    if (o->state != PAL_SOCKET_ST_CONNECTED) {
        SOCKET_LOG(Error, o, "Socket not connected.");
        return PAL_ERR_INVALID_STATE;
    }

    if (o->state == PAL_SOCKET_ST_HANDSHAKED) {
        SOCKET_LOG(Error, o, "Already handshaked.");
        return PAL_ERR_INVALID_STATE;
    }

    pal_err err = o->bio_method.handshake(o->bio_ctx);
    switch (err) {
    case PAL_ERR_OK:
        SOCKET_LOG(Debug, o, "Handshaked.");
        break;
    case PAL_ERR_WANT_READ:
    case PAL_ERR_WANT_WRITE:
        pal_socket_update_interests(o, err == PAL_ERR_WANT_READ ? true : false,
            PAL_ERR_WANT_WRITE ? true : false);
        o->state = PAL_SOCKET_ST_HANDSHAKING;
        o->cb = handshaked_cb;
        o->cb_arg = arg;
        err = PAL_ERR_IN_PROGRESS;
        SOCKET_LOG(Debug, o, "Handshaking ...");
        break;
    default:
        break;
    }

    return err;
}

void pal_socket_set_bio(pal_socket_obj *_o, void *ctx, const pal_socket_bio_method *method) {
    HAPPrecondition(_o);
    HAPPrecondition(ctx);
    HAPPrecondition(method);
    HAPPrecondition(method->handshake);
    HAPPrecondition(method->recv);
    HAPPrecondition(method->send);
    HAPPrecondition(method->pending);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    o->bio_ctx = ctx;
    o->bio_method = *method;
}

pal_err pal_socket_raw_send(pal_socket_obj *_o, const void *data, size_t *len) {
    HAPPrecondition(_o);
    HAPPrecondition(data);
    HAPPrecondition(len);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    return pal_socket_raw_sendto(o, data, len, NULL);
}

pal_err pal_socket_raw_recv(pal_socket_obj *_o, void *buf, size_t *len) {
    HAPPrecondition(_o);
    HAPPrecondition(buf);
    HAPPrecondition(len);

    pal_socket_obj_int *o = (pal_socket_obj_int *)_o;
    HAPAssert(o->magic == PAL_SOCKET_OBJ_MAGIC);

    return pal_socket_raw_recvfrom(o, buf, len, NULL);
}
