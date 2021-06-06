// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pal/net/udp.h>
#include <pal/memory.h>

#include <HAPLog.h>
#include <HAPPlatform.h>

struct pal_net_udp {
    bool bound;
    size_t id;
    int fd;
    pal_net_domain domain;
};

/**
 * Log with type.
 *
 * @param type [Debug|Info|Default|Error|Fault]
 */
#define UDP_LOG(type, udp, fmt, arg...) \
    HAPLogWithType(&udp_log_obj, kHAPLogType_ ## type, \
    "(id=%u) " fmt, udp ? udp->id : 0, ##arg)

static const HAPLogObject udp_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "udp",
};

static size_t gudp_pcb_count;

pal_net_udp *pal_net_udp_new(pal_net_domain domain) {
    pal_net_udp *udp = pal_mem_calloc(sizeof(*udp));
    if (!udp) {
        UDP_LOG(Error, udp, "%s: Failed to calloc memory.", __func__);
        return NULL;
    }

    int _domain;
    switch (domain) {
    case PAL_NET_DOMAIN_INET:
        _domain = AF_INET;
    case PAL_NET_DOMAIN_INET6:
        _domain = AF_INET6;
    default:
        HAPAssertionFailure();
    };

    udp->id = ++gudp_pcb_count;

    udp->fd = socket(_domain, SOCK_DGRAM, 0);
    if (udp->fd == -1) {
        UDP_LOG(Error, udp, "%s: socket() = %s", __func__, strerror(errno));
        goto clean;
    }
    udp->domain = domain;
    UDP_LOG(Debug, udp, "%s() = %p", __func__, udp);
    return udp;
clean:
    pal_mem_free(udp);
    return NULL;
}

static bool
pal_net_addr_ipv4_get(struct sockaddr_in *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin_family = AF_INET;
    in_addr_t addr = inet_addr(src_addr);
    if (addr == INADDR_NONE) {
        return false;
    }
    dst_addr->sin_port = port;
    dst_addr->sin_addr.s_addr = addr;
    return true;
}

static bool
pal_net_addr_ipv6_get(struct sockaddr_in6 *dst_addr, const char *src_addr, uint16_t port) {
    dst_addr->sin6_family = AF_INET6;
    struct in6_addr addr;
    int ret = inet_pton(AF_INET6, src_addr, &addr);
    if (ret) {
        return false;
    }
    dst_addr->sin6_port = port;
    dst_addr->sin6_addr = addr;
    return true;
}

pal_net_err pal_net_udp_bind(pal_net_udp *udp, const char *addr, uint16_t port) {
    HAPPrecondition(udp);
    HAPPrecondition(addr);

    int ret;
    switch (udp->domain) {
    case PAL_NET_DOMAIN_INET: {
        struct sockaddr_in sock_addr;
        if (!pal_net_addr_ipv4_get(&sock_addr, addr, port)) {
            return PAL_NET_ERR_UNKNOWN;
        }
        ret = bind(udp->fd, &sock_addr, sizeof(sock_addr));
        break;
    }
    case PAL_NET_DOMAIN_INET6: {
        struct sockaddr_in6 sock_addr;
        if (!pal_net_addr_ipv6_get(&sock_addr, addr, port)) {
            return PAL_NET_ERR_UNKNOWN;
        }
        ret = bind(udp->fd, &sock_addr, sizeof(sock_addr));
        break;
    }
    default:
        HAPAssertionFailure();
    };
    if (ret == -1) {
        UDP_LOG(Error, udp, "%s: bind() error.", __func__);
        return PAL_NET_ERR_UNKNOWN;
    }
    udp->bound = true;
    UDP_LOG(Debug, udp, "Already bound to %s:%u", __func__, addr, port);
    return PAL_NET_ERR_OK;
}

pal_net_err pal_net_udp_connect(pal_net_udp *udp, const char *addr, uint16_t port) {

}

pal_net_err pal_net_udp_send(pal_net_udp *udp, const void *data, size_t len) {

}

void pal_net_udp_set_recv_cb(pal_net_udp *udp, pal_net_udp_recv_cb cb, void *recv_arg) {

}

void pal_net_udp_free(pal_net_udp *udp) {
    if (udp) {
        close(udp->fd);
        pal_mem_free(udp);
    }
}
