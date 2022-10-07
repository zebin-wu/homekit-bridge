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
#include <pal/net_if.h>
#include <pal/net_addr_int.h>
#include <pal/mem.h>

struct event_cb_desc {
    pal_net_if_event event;
    pal_net_if_event_cb cb;
    void *arg;
    LIST_ENTRY(event_cb_desc) list_entry;
};

static int gfd;
static LIST_HEAD(event_cb_desc_list_head, event_cb_desc)
    gevent_cb_desc_list_heads[PAL_NET_IF_EVENT_COUNT];

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

    return sendmsg(fd, &msg, 0);
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
    return ret;
}

void pal_net_if_init() {
    for (size_t i = 0; i < PAL_NET_IF_EVENT_COUNT; i++) {
        LIST_INIT(&gevent_cb_desc_list_heads[i]);
    }

    gfd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (gfd == -1) {
        HAPFatalError();
    }
}

void pal_net_if_deinit() {
    close(gfd);
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

    char resp[1024];

    do {
        int len = netlink_response(fd, resp, sizeof(resp));
        if (len < 0) {
            err = PAL_ERR_UNKNOWN;
            goto end;
        }

        for (struct nlmsghdr *hdr = (struct nlmsghdr *)resp; NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len)) {
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
