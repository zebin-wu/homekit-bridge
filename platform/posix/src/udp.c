// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pal/udp.h>
#include <pal/memory.h>

#include <HAPLog.h>
#include <HAPPlatform.h>
#include <HAPPlatformFileHandle.h>

/**
 * Log with type.
 *
 * @param type [Debug|Info|Default|Error|Fault]
 * @param udp The pointer to udp object.
 */
#define UDP_LOG(type, udp, fmt, arg...) \
    HAPLogWithType(&udp_log_obj, kHAPLogType_ ## type, \
    "(id=%u) " fmt, (udp) ? (udp)->id : 0, ##arg)

#define UDP_LOG_ERRNO(udp, func) \
    UDP_LOG(Error, udp, "%s: %s() error: %s.", __func__, func, strerror(errno))

typedef struct pal_udp_mbuf {
    char to_addr[PAL_NET_ADDR_MAX_LEN];
    uint16_t to_port;
    struct pal_udp_mbuf *next;
    size_t len;
    char buf[0];
} pal_udp_mbuf;

struct pal_udp {
    bool bound:1;
    bool connected:1;
    uint16_t id;
    int fd;
    pal_net_domain domain;
    char remote_addr[PAL_NET_ADDR_MAX_LEN];
    uint16_t remote_port;
    pal_udp_mbuf *mbuf_list_head;
    pal_udp_mbuf **mbuf_list_ptail;

    HAPPlatformFileHandleRef handle;
    HAPPlatformFileHandleEvent interests;

    pal_udp_recv_cb recv_cb;
    void *recv_arg;

    pal_udp_err_cb err_cb;
    void *err_arg;
};

static const HAPLogObject udp_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "UDP",
};

static size_t gudp_pcb_count;

static void pal_udp_file_handle_callback(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void* context);

static bool
pal_net_addr_get_ipv4(struct sockaddr_in *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin_family = AF_INET;
    int ret = inet_pton(AF_INET, src_addr, &dst_addr->sin_addr.s_addr);
    if (ret <= 0) {
        return false;
    }
    dst_addr->sin_port = htons(port);
    return true;
}

static bool
pal_net_addr_get_ipv6(struct sockaddr_in6 *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin6_family = AF_INET6;
    int ret = inet_pton(AF_INET6, src_addr, &dst_addr->sin6_addr);
    if (ret <= 0) {
        return false;
    }
    dst_addr->sin6_port = htons(port);
    return true;
}

static void pal_udp_add_mbuf(pal_udp *udp, pal_udp_mbuf *mbuf) {
    mbuf->next = NULL;
    *(udp->mbuf_list_ptail) = mbuf;
    udp->mbuf_list_ptail = &mbuf->next;
}

static pal_udp_mbuf *pal_udp_get_mbuf(pal_udp *udp) {
    pal_udp_mbuf *mbuf = udp->mbuf_list_head;
    if (mbuf) {
        udp->mbuf_list_head = mbuf->next;
        if (udp->mbuf_list_head == NULL) {
            udp->mbuf_list_ptail = &udp->mbuf_list_head;
        }
    }
    return mbuf;
}

static void pal_udp_del_mbuf_list(pal_udp *udp) {
    pal_udp_mbuf *cur;
    while (udp->mbuf_list_head) {
        cur = udp->mbuf_list_head;
        udp->mbuf_list_head = cur->next;
        pal_mem_free(cur);
    }
    udp->mbuf_list_ptail = &udp->mbuf_list_head;
}

static bool pal_udp_socket_writable(pal_udp *udp) {
    fd_set write_fds;
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };
    FD_ZERO(&write_fds);
    FD_SET(udp->fd, &write_fds);
    return select(1, NULL, &write_fds, NULL, &tv) == 1 && FD_ISSET(udp->fd, &write_fds);
}

static void pal_udp_raw_recv(pal_udp *udp) {
    pal_net_err err = PAL_NET_ERR_OK;
    char buf[1500];  // Same size as MTU
    char from_addr[PAL_NET_ADDR_MAX_LEN] = { 0 };
    uint16_t from_port;
    ssize_t rc;
    if (udp->connected) {
        do {
            rc = recv(udp->fd, buf, sizeof(buf), 0);
        } while (rc == -1 && errno == EINTR);
        // UDP protocol allows to send/receive a 0 byte packet.
        if (rc == -1) {
            UDP_LOG_ERRNO(udp, "recv");
            err = PAL_NET_ERR_UNKNOWN;
            goto err;
        }
        HAPRawBufferCopyBytes(from_addr, udp->remote_addr, PAL_NET_ADDR_MAX_LEN);
        from_port = udp->remote_port;
    } else {
        switch (udp->domain) {
        case PAL_NET_DOMAIN_INET: {
            struct sockaddr_in sa;
            socklen_t addr_len = sizeof(sa);
            do {
                rc = recvfrom(udp->fd, buf, sizeof(buf),
                    0, (struct sockaddr *)&sa, &addr_len);
            } while (rc == -1 && errno == EINTR);
            if (rc == - 1) {
                UDP_LOG_ERRNO(udp, "recvfrom");
                err = PAL_NET_ERR_UNKNOWN;
                goto err;
            }
            from_port = ntohs(sa.sin_port);
            inet_ntop(AF_INET, &sa.sin_addr.s_addr,
                from_addr, sizeof(from_addr));
            break;
        }
        case PAL_NET_DOMAIN_INET6: {
            struct sockaddr_in6 sa;
            socklen_t addr_len = sizeof(sa);
            do {
                rc = recvfrom(udp->fd, buf, sizeof(buf),
                    0, (struct sockaddr *)&sa, &addr_len);
            } while (rc == -1 && errno == EINTR);
            if (rc == -1) {
                UDP_LOG_ERRNO(udp, "recvfrom");
                err = PAL_NET_ERR_UNKNOWN;
                goto err;
            }
            from_port = ntohs(sa.sin6_port);
            inet_ntop(AF_INET, &sa.sin6_addr,
                from_addr, sizeof(from_addr));
            break;
        }
        default:
            HAPAssertionFailure();
        }
    }
    HAPLogBufferDebug(&udp_log_obj, buf, rc, "(id=%u) Receive packet(len=%zd) from %s:%u",
        udp->id, rc, from_addr, from_port);
    if (udp->recv_cb) {
        udp->recv_cb(udp, buf, rc, from_addr, from_port, udp->recv_arg);
    }
    return;

err:
    if (udp->err_cb) {
        udp->err_cb(udp, err, udp->err_arg);
    }
}

static pal_net_err pal_udp_send_sync(pal_udp *udp, const char *addr, uint16_t port, const void *data, size_t len) {
    ssize_t rc;

    if (addr) {
        switch (udp->domain) {
        case PAL_NET_DOMAIN_INET: {
            struct sockaddr_in sa;
            if (!pal_net_addr_get_ipv4(&sa, addr, port)) {
                UDP_LOG(Error, udp, "%s: Invalid address \"%s\".",
                    __func__, addr);
                return PAL_NET_ERR_UNKNOWN;
            }
            do {
                rc = sendto(udp->fd, data, len, 0,
                    (struct sockaddr *)&sa, sizeof(sa));
            } while (rc == -1 && errno == EINTR);
            break;
        }
        case PAL_NET_DOMAIN_INET6: {
            struct sockaddr_in6 sa;
            if (!pal_net_addr_get_ipv6(&sa, addr, port)) {
                UDP_LOG(Error, udp, "%s: Invalid address \"%s\".",
                    __func__, addr);
                return PAL_NET_ERR_UNKNOWN;
            }
            do {
                rc = sendto(udp->fd, data, len, 0,
                    (struct sockaddr *)&sa, sizeof(sa));
            } while (rc == -1 && errno == EINTR);
            break;
        }
        default:
            HAPAssertionFailure();
        }
    } else {
        do {
            rc = send(udp->fd, data, len, 0);
        } while (rc == -1 && errno == EINTR);
    }
    if (rc != len) {
        if (rc == -1) {
            UDP_LOG_ERRNO(udp, addr ? "sendto" : "send");
        } else {
            UDP_LOG(Error, udp, "%s: Only sent %zd byte.", __func__, rc);
        }
        return PAL_NET_ERR_UNKNOWN;
    }
    HAPLogBufferDebug(&udp_log_obj, data, len,
        "(id=%u) Sent packet(len=%zd) to %s:%u", udp->id,
        len, addr ? addr : udp->remote_addr,
        addr ? port : udp->remote_port);
    return PAL_NET_ERR_OK;
}

static void pal_udp_raw_send(pal_udp *udp) {
    pal_net_err err = PAL_NET_ERR_OK;
    pal_udp_mbuf *mbuf = pal_udp_get_mbuf(udp);
    if (!mbuf) {
        err = PAL_NET_ERR_UNKNOWN;
        goto end;
    }
    if (udp->mbuf_list_head == NULL) {
        udp->interests.isReadyForWriting = false;
        HAPPlatformFileHandleUpdateInterests(udp->handle, udp->interests,
            pal_udp_file_handle_callback, udp);
    }

    err = pal_udp_send_sync(udp, mbuf->to_addr[0] ? mbuf->to_addr : NULL, mbuf->to_port, mbuf->buf, mbuf->len);
    pal_mem_free(mbuf);

end:
    if (err != PAL_NET_ERR_OK && udp->err_cb) {
        udp->err_cb(udp, err, udp->err_arg);
    }
}

static void pal_udp_raw_exception(pal_udp *udp) {
    UDP_LOG(Error, udp, "%s", __func__);
    if (udp->err_cb) {
        udp->err_cb(udp, PAL_NET_ERR_UNKNOWN, udp->err_arg);
    }
}

static void pal_udp_file_handle_callback(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void* context) {
    HAPPrecondition(context);

    pal_udp *udp = context;
    HAPAssert(udp->handle == fileHandle);

    if (fileHandleEvents.hasErrorConditionPending) {
        pal_udp_raw_exception(udp);
    } else if (fileHandleEvents.isReadyForReading) {
        pal_udp_raw_recv(udp);
    } else if (fileHandleEvents.isReadyForWriting) {
        pal_udp_raw_send(udp);
    }
}

pal_udp *pal_udp_new(pal_net_domain domain) {
    pal_udp *udp = pal_mem_calloc(sizeof(*udp));
    if (!udp) {
        UDP_LOG(Error, udp, "%s: Failed to calloc memory.", __func__);
        return NULL;
    }

    int _domain;
    switch (domain) {
    case PAL_NET_DOMAIN_INET:
        _domain = AF_INET;
        break;
    case PAL_NET_DOMAIN_INET6:
        _domain = AF_INET6;
        break;
    default:
        HAPAssertionFailure();
    }

    udp->id = ++gudp_pcb_count;

    udp->fd = socket(_domain, SOCK_DGRAM, IPPROTO_UDP);
    if (udp->fd == -1) {
        UDP_LOG_ERRNO(udp, "socket");
        pal_mem_free(udp);
        return NULL;
    }
    udp->mbuf_list_ptail = &udp->mbuf_list_head;
    udp->domain = domain;
    udp->interests.isReadyForReading = true;
    udp->interests.hasErrorConditionPending = true;
    HAPError err = HAPPlatformFileHandleRegister(&udp->handle, udp->fd,
        udp->interests, pal_udp_file_handle_callback, udp);
    if (err != kHAPError_None) {
        UDP_LOG(Error, udp, "%s: Failed to register handle callback", __func__);
        return NULL;
    }
    UDP_LOG(Debug, udp, "%s() = %p", __func__, udp);
    return udp;
}

pal_net_err pal_udp_enable_broadcast(pal_udp *udp) {
    HAPPrecondition(udp);

    int optval = 1;
    int ret = setsockopt(udp->fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (ret) {
        return PAL_NET_ERR_UNKNOWN;
    }
    return PAL_NET_ERR_OK;
}

pal_net_err pal_udp_bind(pal_udp *udp, const char *addr, uint16_t port) {
    HAPPrecondition(udp);
    HAPPrecondition(addr);

    int ret;
    switch (udp->domain) {
    case PAL_NET_DOMAIN_INET: {
        struct sockaddr_in sa;
        if (!pal_net_addr_get_ipv4(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        ret = bind(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    case PAL_NET_DOMAIN_INET6: {
        struct sockaddr_in6 sa;
        if (!pal_net_addr_get_ipv6(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        ret = bind(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    default:
        HAPAssertionFailure();
    }
    if (ret == -1) {
        UDP_LOG_ERRNO(udp, "bind");
        return PAL_NET_ERR_UNKNOWN;
    }
    udp->bound = true;
    UDP_LOG(Debug, udp, "Bound to %s:%u", addr, port);
    return PAL_NET_ERR_OK;
}

pal_net_err pal_udp_connect(pal_udp *udp, const char *addr, uint16_t port) {
    size_t addr_len = HAPStringGetNumBytes(addr);

    HAPPrecondition(udp);
    HAPPrecondition(addr);
    HAPPrecondition(addr_len < PAL_NET_ADDR_MAX_LEN);

    int ret;
    switch (udp->domain) {
    case PAL_NET_DOMAIN_INET: {
        struct sockaddr_in sa;
        if (!pal_net_addr_get_ipv4(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        ret = connect(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    case PAL_NET_DOMAIN_INET6: {
        struct sockaddr_in6 sa;
        if (!pal_net_addr_get_ipv6(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        ret = connect(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
        break;
    }
    default:
        HAPAssertionFailure();
    }
    if (ret == -1) {
        UDP_LOG_ERRNO(udp, "connect");
        return PAL_NET_ERR_UNKNOWN;
    }
    HAPRawBufferCopyBytes(udp->remote_addr, addr, addr_len + 1);
    udp->remote_port = port;
    udp->connected = true;
    UDP_LOG(Debug, udp, "Connected to %s:%u", addr, port);
    return PAL_NET_ERR_OK;
}

pal_net_err pal_udp_send(pal_udp *udp, const void *data, size_t len) {
    HAPPrecondition(udp);
    if (len > 0) {
        HAPPrecondition(data);
    }

    if (!udp->connected) {
        UDP_LOG(Error, udp, "%s: Unknown remote address and port, connect first.", __func__);
        return PAL_NET_ERR_NOT_CONN;
    }

    if (pal_udp_socket_writable(udp)) {
        return pal_udp_send_sync(udp, NULL, 0, data, len);
    }

    pal_udp_mbuf *mbuf = pal_mem_alloc(sizeof(*mbuf) + len);
    if (!mbuf) {
        UDP_LOG(Error, udp, "%s: Failed to alloc memory.", __func__);
        return PAL_NET_ERR_ALLOC;
    }
    HAPRawBufferCopyBytes(mbuf->buf, data, len);
    mbuf->len = len;
    mbuf->to_addr[0] = '\0';
    mbuf->to_port = 0;
    pal_udp_add_mbuf(udp, mbuf);
    if (!udp->interests.isReadyForWriting) {
        udp->interests.isReadyForWriting = true;
        HAPPlatformFileHandleUpdateInterests(udp->handle, udp->interests,
            pal_udp_file_handle_callback, udp);
    }
    UDP_LOG(Debug, udp, "%s(len = %zu)", __func__, len);
    return PAL_NET_ERR_OK;
}

pal_net_err pal_udp_sendto(pal_udp *udp, const void *data, size_t len,
    const char *addr, uint16_t port) {
    size_t addr_len = HAPStringGetNumBytes(addr);

    HAPPrecondition(udp);
    HAPPrecondition(addr);
    HAPPrecondition(addr_len < PAL_NET_ADDR_MAX_LEN);
    if (len > 0) {
        HAPPrecondition(data);
    }

    if (pal_udp_socket_writable(udp)) {
        return pal_udp_send_sync(udp, addr, port, data, len);
    }

    switch (udp->domain) {
    case PAL_NET_DOMAIN_INET: {
        struct sockaddr_in sa;
        if (!pal_net_addr_get_ipv4(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        break;
    }
    case PAL_NET_DOMAIN_INET6: {
        struct sockaddr_in6 sa;
        if (!pal_net_addr_get_ipv6(&sa, addr, port)) {
            UDP_LOG(Error, udp, "%s: Invalid address \"%s\".", __func__, addr);
            return PAL_NET_ERR_INVALID_ARG;
        }
        break;
    }
    default:
        HAPAssertionFailure();
    }

    pal_udp_mbuf *mbuf = pal_mem_alloc(sizeof(*mbuf) + len);
    if (!mbuf) {
        UDP_LOG(Error, udp, "%s: Failed to alloc memory.", __func__);
        return PAL_NET_ERR_ALLOC;
    }
    HAPRawBufferCopyBytes(mbuf->buf, data, len);
    mbuf->len = len;
    HAPRawBufferCopyBytes(mbuf->to_addr, addr, addr_len);
    mbuf->to_addr[addr_len] = '\0';
    mbuf->to_port = port;
    pal_udp_add_mbuf(udp, mbuf);
    if (!udp->interests.isReadyForWriting) {
        udp->interests.isReadyForWriting = true;
        HAPPlatformFileHandleUpdateInterests(udp->handle, udp->interests,
            pal_udp_file_handle_callback, udp);
    }
    UDP_LOG(Debug, udp, "%s(len = %zu, addr = %s, port = %u)", __func__, len, addr, port);
    return PAL_NET_ERR_OK;
}

void pal_udp_set_recv_cb(pal_udp *udp, pal_udp_recv_cb cb, void *arg) {
    HAPPrecondition(udp);

    udp->recv_cb = cb;
    udp->recv_arg = arg;
}

void pal_udp_set_err_cb(pal_udp *udp, pal_udp_err_cb cb, void *arg) {
    HAPPrecondition(udp);

    udp->err_cb = cb;
    udp->err_arg = arg;
}

void pal_udp_free(pal_udp *udp) {
    if (!udp) {
        return;
    }
    UDP_LOG(Debug, udp, "%s(%p)", __func__, udp);
    HAPPlatformFileHandleDeregister(udp->handle);
    close(udp->fd);
    pal_udp_del_mbuf_list(udp);
    pal_mem_free(udp);
}
