// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <HAPPlatformFileHandle.h>
#include <pal/net_if.h>
#include <pal/net_addr_int.h>
#include <pal/mem.h>

#define PAL_NET_IF_BUFLEN 512

#define NET_IF_LOG_ERRNO(func) \
    HAPLogError(&logObject, "%s: %s() failed: %s", __func__, #func, strerror(errno));

struct event_cb_desc {
    pal_net_if_event event;
    pal_net_if_event_cb cb;
    void *arg;
    LIST_ENTRY(event_cb_desc) list_entry;
};

struct net_if_state {
    int event_fd;
    HAPPlatformFileHandleRef event_handle;
    LIST_HEAD(, event_cb_desc) event_cb_desc_list_heads[PAL_NET_IF_EVENT_COUNT];
};

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "netif" };

static bool ginited;
static struct net_if_state gstate;

static ssize_t netlink_request(int fd, void *req, size_t reqlen) {
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec vec;

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_groups = 0;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    vec.iov_base = req;
    vec.iov_len = reqlen;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;

    int ret = sendmsg(fd, &msg, 0);
    if (ret < 0) {
        NET_IF_LOG_ERRNO(sendmsg);
    }
    return ret;
}

static ssize_t netlink_response(int fd, void *resp, size_t resplen) {
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec vec;
    ssize_t ret;

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_groups = 0;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    vec.iov_base = resp;
    vec.iov_len = resplen;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;

    ret = recvmsg(fd, &msg, 0);
    if (msg.msg_flags & MSG_TRUNC) {
        return -1;
    }
    if (ret < 0) {
        NET_IF_LOG_ERRNO(recvmsg);
    }
    return ret;
}

static void pal_net_if_handle_link_event(struct nlmsghdr *hdr) {
    HAPLogDebug(&logObject, "Got netif link event.");
}

static void pal_net_if_handle_addr_event(struct nlmsghdr *hdr) {
    HAPLogDebug(&logObject, "Got netif address event.");
}

static void pal_net_if_handle_route_event(struct nlmsghdr *hdr) {
    HAPLogDebug(&logObject, "Got netif route event.");
}

static void pal_net_if_handle_event_cb(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void* _Nullable context) {
    HAPPrecondition(fileHandle == gstate.event_handle);
    HAPPrecondition(fileHandleEvents.isReadyForReading);

    bool loop = true;
    struct sockaddr_nl addr;
    char buf[PAL_NET_IF_BUFLEN];

    do {
        socklen_t addrlen = sizeof(addr);
        int len = recvfrom(gstate.event_fd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
        if (len < 0) {
            NET_IF_LOG_ERRNO(recvfrom);
            break;
        }

        for (struct nlmsghdr *hdr = (struct nlmsghdr *)buf;
            loop && NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len)) {
            switch (hdr->nlmsg_type) {
            case NLMSG_DONE:
            case NLMSG_ERROR:
                loop = false;
                break;
            case RTM_NEWLINK:
            case RTM_DELLINK:
                pal_net_if_handle_link_event(hdr);
                break;
            case RTM_NEWADDR:
            case RTM_DELADDR:
                pal_net_if_handle_addr_event(hdr);
                break;
            case RTM_NEWROUTE:
            case RTM_DELROUTE:
                pal_net_if_handle_route_event(hdr);
                break;
            default:
                break;
            }
        }
    } while (loop);
}

void pal_net_if_init() {
    HAPPrecondition(ginited == false);

    for (size_t i = 0; i < PAL_NET_IF_EVENT_COUNT; i++) {
        LIST_INIT(&gstate.event_cb_desc_list_heads[i]);
    }

    gstate.event_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (gstate.event_fd == -1) {
        HAPFatalError();
    }

    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;
    int ret = bind(gstate.event_fd, (struct sockaddr *)&sa, sizeof(sa));
    if (ret == -1) {
        HAPFatalError();
    }

    if (HAPPlatformFileHandleRegister(&gstate.event_handle, gstate.event_fd, (HAPPlatformFileHandleEvent) {
        .hasErrorConditionPending = false,
        .isReadyForReading = true,
        .isReadyForWriting = false,
    }, pal_net_if_handle_event_cb, NULL) != kHAPError_None) {
        HAPFatalError();
    }

    ginited = true;
}

void pal_net_if_deinit() {
    HAPPrecondition(ginited);

    HAPPlatformFileHandleDeregister(gstate.event_handle);
    close(gstate.event_fd);

    ginited = false;
}

pal_net_if *pal_net_if_find(const char *name) {
    HAPPrecondition(name);

    return (pal_net_if *)(uintptr_t)if_nametoindex(name);
}

pal_err pal_net_if_foreach(pal_err (*func)(pal_net_if *netif, void *arg), void *arg) {
    HAPPrecondition(func);

    struct if_nameindex *arr = if_nameindex();
    if (!arr) {
        return PAL_ERR_UNKNOWN;
    }

    pal_err ret = PAL_ERR_OK;
    for (int i = 0; arr[i].if_index; i++) {
        ret = func((pal_net_if *)(uintptr_t)arr[i].if_index, arg);
        if (ret != PAL_ERR_OK) {
            goto end;
        }
    }

end:
    if_freenameindex(arr);
    return ret;
}

pal_err pal_net_if_get_name(pal_net_if *netif, char name[PAL_NET_IF_NAME_MAX_LEN]) {
    HAPPrecondition(netif);
    HAPPrecondition(name);

    if (if_indextoname((unsigned int)(uintptr_t)netif, name)) {
        return PAL_ERR_OK;
    }

    switch (errno) {
    case ENXIO:
        return PAL_ERR_INVALID_ARG;
    default:
        return PAL_ERR_UNKNOWN;
    }
}

pal_err pal_net_if_get_hw_addr(pal_net_if *netif, uint8_t addr[PAL_NET_IF_HWADDR_MAX_LEN]) {
    HAPPrecondition(netif);
    HAPPrecondition(addr);

    char name[IF_NAMESIZE];
    pal_err err = pal_net_if_get_name(netif, name);
    if (err != PAL_ERR_OK) {
        return err;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return PAL_ERR_UNKNOWN;
    }

    struct ifreq req;
    strncpy(req.ifr_name, name, sizeof(req.ifr_name));
    int ret = ioctl(fd, SIOCGIFHWADDR, &req);
    close(fd);
    if (ret == -1) {
        return PAL_ERR_UNKNOWN;
    }
    memcpy(addr, req.ifr_hwaddr.sa_data, 6);
    return PAL_ERR_OK;
}

pal_err pal_net_if_get_ipv4_addr(pal_net_if *netif, pal_net_addr *_addr) {
    HAPPrecondition(netif);
    HAPPrecondition(_addr);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    char name[IF_NAMESIZE];
    pal_err err = pal_net_if_get_name(netif, name);
    if (err != PAL_ERR_OK) {
        return err;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return PAL_ERR_UNKNOWN;
    }

    struct ifreq req;
    strncpy(req.ifr_name, name, sizeof(req.ifr_name));
    int ret = ioctl(fd, SIOCGIFADDR, &req);
    close(fd);
    if (ret == -1) {
        return PAL_ERR_UNKNOWN;
    }
    addr->family = PAL_NET_ADDR_FAMILY_INET;
    addr->u.in = ((struct sockaddr_in *) &req.ifr_addr)->sin_addr;
    return PAL_ERR_OK;
}

pal_err pal_net_if_get_ipv4_netmask(pal_net_if *netif, pal_net_addr *_addr) {
    HAPPrecondition(netif);
    HAPPrecondition(_addr);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    char name[IF_NAMESIZE];
    pal_err err = pal_net_if_get_name(netif, name);
    if (err != PAL_ERR_OK) {
        return err;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return PAL_ERR_UNKNOWN;
    }

    struct ifreq req;
    strncpy(req.ifr_name, name, sizeof(req.ifr_name));
    int ret = ioctl(fd, SIOCGIFNETMASK, &req);
    close(fd);
    if (ret == -1) {
        return PAL_ERR_UNKNOWN;
    }
    addr->family = PAL_NET_ADDR_FAMILY_INET;
    addr->u.in = ((struct sockaddr_in *) &req.ifr_netmask)->sin_addr;
    return PAL_ERR_OK;
}

pal_err pal_net_if_ipv6_addr_foreach(
        pal_net_if *netif,
        pal_err (*func)(pal_net_if *netif, pal_net_addr *addr, void *arg),
        void *arg) {
    HAPPrecondition(netif);
    HAPPrecondition(func);

    int fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (fd == -1) {
        NET_IF_LOG_ERRNO(socket);
        return PAL_ERR_UNKNOWN;
    }

    pal_err err = PAL_ERR_OK;
    size_t reqlen = NLMSG_SPACE(sizeof(struct ifaddrmsg));
    char req[reqlen];
    memset(req, 0, reqlen);
    struct nlmsghdr *hdr = (struct nlmsghdr *)req;
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    hdr->nlmsg_type = RTM_GETADDR;
    struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(hdr);
    ifa->ifa_family = AF_INET6;
    ifa->ifa_index = (uintptr_t)netif;

    if (netlink_request(fd, req, reqlen) < 0) {
        err = PAL_ERR_UNKNOWN;
        goto end;
    }

    char resp[PAL_NET_IF_BUFLEN];

    do {
        int len = netlink_response(fd, resp, sizeof(resp));
        if (len < 0) {
            err = PAL_ERR_UNKNOWN;
            goto end;
        }

        for (struct nlmsghdr *hdr = (struct nlmsghdr *)resp;
            NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len)) {
            struct rtattr *attr;
            ssize_t attrlen;

            if (hdr->nlmsg_type == NLMSG_DONE) {
                goto end;
            }

            if (hdr->nlmsg_type == NLMSG_ERROR) {
                err = PAL_ERR_UNKNOWN;
                goto end;
            }

            ifa = (struct ifaddrmsg *)NLMSG_DATA(hdr);
            if (ifa->ifa_family != AF_INET6 || ifa->ifa_index != (uintptr_t)netif) {
                continue;
            }

            attr = IFA_RTA(ifa);
            attrlen = RTM_PAYLOAD(hdr);
            for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
                if (attr->rta_type == IFA_ADDRESS) {
                    pal_net_addr_int addr;
                    addr.family = PAL_NET_ADDR_FAMILY_INET6;
                    addr.u.in6 = *((struct in6_addr *)RTA_DATA(attr));
                    err = func(netif, (pal_net_addr *)&addr, arg);
                    if (err != PAL_ERR_OK) {
                        goto end;
                    }
                }
            }
        }
    } while (1);

end:
    close(fd);
    return err;
}

void pal_net_if_register_event(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb) {
}

void pal_net_if_unregister_event(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb) {
}
