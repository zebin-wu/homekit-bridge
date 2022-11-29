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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <HAPPlatformFileHandle.h>
#include <pal/net_if.h>
#include <pal/net_addr_int.h>
#include <pal/mem.h>

#define PAL_NET_IF_BUFLEN 2048

#define NET_IF_LOG_ERRNO(func) \
    HAPLogError(&logObject, "%s: %s() failed: %s", __func__, #func, strerror(errno));

#define PAL_NET_IF_MAGIC 0x5555U

#define FLAG_ISSET(flags, flag) (((flags) & (flag)) != 0)

struct event_cb_desc {
    pal_net_if *netif;
    pal_net_if_event_cb cb;
    void *arg;
    LIST_ENTRY(event_cb_desc) list_entry;
};

struct pal_net_if_ipv6_addr {
    pal_net_addr_int addr;
    TAILQ_ENTRY(pal_net_if_ipv6_addr) list_entry;
};

struct pal_net_if {
    uint16_t magic;
    bool up:1;
    bool loopback:1;
    int index;
    char name[IF_NAMESIZE];
    char hw_addr[PAL_NET_IF_HW_ADDR_LEN];
    pal_net_addr_int ipv4_addr;
    TAILQ_HEAD(, pal_net_if_ipv6_addr) ipv6_addrs;
    TAILQ_ENTRY(pal_net_if) list_entry;
};

struct net_if_state {
    int event_fd;
    HAPPlatformFileHandleRef event_handle;
    TAILQ_HEAD(, pal_net_if) net_ifs;
    LIST_HEAD(, event_cb_desc) event_cbs[PAL_NET_IF_EVENT_COUNT];
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

static void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr, int len) {
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type <= max) {
            tb[attr->rta_type] = attr;
        }
    }
}

static pal_net_if *pal_net_if_find_by_index(int index) {
    pal_net_if *netif;
    TAILQ_FOREACH(netif, &gstate.net_ifs, list_entry) {
        if (netif->index == index) {
            return netif;
        }
    }
    return NULL;
}

static void pal_net_if_call_event_handlers(pal_net_if *netif, pal_net_if_event event) {
    struct event_cb_desc *desc;
    LIST_FOREACH(desc, gstate.event_cbs + event, list_entry) {
        if (desc->netif && desc->netif != netif) {
            continue;
        }
        desc->cb(netif, event, desc->arg);
    }
}

static int pal_net_if_get_config(const char *name, int fd, unsigned long request, struct ifreq *ifr) {
    memset(ifr, 0, sizeof(*ifr));
    strncpy(ifr->ifr_name, name, sizeof(ifr->ifr_name));
    return ioctl(fd, request, ifr);
}

static pal_net_if *pal_net_if_create(int index, const char *name) {
    pal_net_if *netif = pal_mem_calloc(1, sizeof(*netif));
    if (!netif) {
        return NULL;
    }

    netif->magic = PAL_NET_IF_MAGIC;
    netif->index = index;
    strncpy(netif->name, name, sizeof(netif->name));

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        goto err;
    }

    struct ifreq ifr;

    if (pal_net_if_get_config(name, fd, SIOCGIFFLAGS, &ifr)) {
        goto err1;
    }
    netif->up = FLAG_ISSET(ifr.ifr_flags, IFF_UP);
    netif->loopback = FLAG_ISSET(ifr.ifr_flags, IFF_LOOPBACK);

    if (pal_net_if_get_config(name, fd, SIOCGIFHWADDR, &ifr)) {
        goto err1;
    }
    memcpy(netif->hw_addr, ifr.ifr_hwaddr.sa_data, sizeof(netif->hw_addr));

    if (!pal_net_if_get_config(name, fd, SIOCGIFADDR, &ifr)) {
        netif->ipv4_addr.family = PAL_NET_ADDR_FAMILY_INET;
        netif->ipv4_addr.u.in = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
    }

    close(fd);

    TAILQ_INIT(&netif->ipv6_addrs);

    return netif;

err1:
    close(fd);
err:
    pal_mem_free(netif);
    return NULL;
}

static void pal_net_if_handle_link_event(struct nlmsghdr *hdr) {
    HAPLogDebug(&logObject, "Got netif link event.");

    struct ifinfomsg *ifi = NLMSG_DATA(hdr);
    int len = hdr->nlmsg_len - NLMSG_SPACE(sizeof(*ifi));
    struct rtattr *tb[IFLA_MAX + 1];

    memset(tb, 0, sizeof(tb));
    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);

    pal_net_if *netif = pal_net_if_find_by_index(ifi->ifi_index);
    if (!netif) {
        if (hdr->nlmsg_type == RTM_NEWLINK) {
            netif = pal_net_if_create(ifi->ifi_index, RTA_DATA(tb[IFLA_IFNAME]));
            TAILQ_INSERT_TAIL(&gstate.net_ifs, netif, list_entry);
            pal_net_if_call_event_handlers(netif, PAL_NET_IF_EVENT_ADDED);
        }
        return;
    }

    if (hdr->nlmsg_type == RTM_DELLINK) {
        pal_net_if_call_event_handlers(netif, PAL_NET_IF_EVENT_REMOVED);
        TAILQ_REMOVE(&gstate.net_ifs, netif, list_entry);
        for (struct pal_net_if_ipv6_addr *next = TAILQ_FIRST(&netif->ipv6_addrs); next;) {
            struct pal_net_if_ipv6_addr *cur = next;
            next = TAILQ_NEXT(cur, list_entry);
            pal_mem_free(cur);
        }
        pal_mem_free(netif);
        return;
    }

    bool up = (ifi->ifi_flags & IFF_UP) != 0;
    if (netif->up != up) {
        netif->up = up;
        pal_net_if_call_event_handlers(netif, up ? PAL_NET_IF_EVENT_UP : PAL_NET_IF_EVENT_DOWN);
    }
}

static struct pal_net_if_ipv6_addr *pal_net_if_find_ipv6_addr(pal_net_if *netif, struct in6_addr *addr) {
    struct pal_net_if_ipv6_addr *ipv6_addr;
    TAILQ_FOREACH(ipv6_addr, &netif->ipv6_addrs, list_entry) {
        if (!memcmp(&ipv6_addr->addr.u.in6, addr, sizeof(*addr))) {
            return ipv6_addr;
        }
    }
    return NULL;
}

static pal_err pal_net_if_add_ipv6_addr(pal_net_if *netif, struct in6_addr *in6) {
    struct pal_net_if_ipv6_addr *ipv6_addr = pal_mem_alloc(sizeof(*ipv6_addr));
    if (!ipv6_addr) {
        return PAL_ERR_ALLOC;
    }

    ipv6_addr->addr.family = PAL_NET_ADDR_FAMILY_INET6;
    ipv6_addr->addr.u.in6 = *in6;
    TAILQ_INSERT_TAIL(&netif->ipv6_addrs, ipv6_addr, list_entry);
    return PAL_ERR_OK;
}

static bool pal_net_if_has_ipv4_addr(pal_net_if *netif, struct in_addr *in) {
    return memcmp(&netif->ipv4_addr.u.in, in, sizeof(*in)) == 0;
}

static void pal_net_if_set_ipv4_addr(pal_net_if *netif, struct in_addr *in) {
    netif->ipv4_addr.family = PAL_NET_ADDR_FAMILY_INET;
    netif->ipv4_addr.u.in = *in;
}

static void pal_net_if_handle_addr_event(struct nlmsghdr *hdr) {
    HAPLogDebug(&logObject, "Got netif address event.");

    struct ifaddrmsg *ifa = NLMSG_DATA(hdr);
    int len = hdr->nlmsg_len - NLMSG_SPACE(sizeof(*ifa));
    struct rtattr *tb[IFA_MAX + 1];

    memset(tb, 0, sizeof(tb));
    parse_rtattr(tb, IFA_MAX, IFA_RTA(ifa), len);

    if (!tb[IFA_ADDRESS]) {
        return;
    }

    pal_net_if *netif = pal_net_if_find_by_index(ifa->ifa_index);
    if (!netif) {
        HAPLogError(&logObject, "%s: netif %d not found", __func__, ifa->ifa_index);
        return;
    }

    bool changed = false;
    switch (ifa->ifa_family) {
    case AF_INET: {
        bool has_addr = pal_net_if_has_ipv4_addr(netif, RTA_DATA(tb[IFA_ADDRESS]));
        if (has_addr && hdr->nlmsg_type == RTM_DELADDR) {
            changed = true;
            memset(&netif->ipv4_addr, 0, sizeof(pal_net_addr_int));
        }
        if (!has_addr && hdr->nlmsg_type == RTM_NEWADDR) {
            changed = true;
            pal_net_if_set_ipv4_addr(netif, RTA_DATA(tb[IFA_ADDRESS]));
        }
        if (changed) {
            pal_net_if_call_event_handlers(netif, PAL_NET_IF_EVENT_IPV4_ADDR_CHANGED);
        }
    } break;
    case AF_INET6: {
        struct pal_net_if_ipv6_addr *ipv6_addr = pal_net_if_find_ipv6_addr(netif, RTA_DATA(tb[IFA_ADDRESS]));
        if (ipv6_addr) {
            if (hdr->nlmsg_type != RTM_DELADDR) {
                break;
            }
            changed = true;
            TAILQ_REMOVE(&netif->ipv6_addrs, ipv6_addr, list_entry);
            pal_mem_free(ipv6_addr);
            break;
        } else if (hdr->nlmsg_type == RTM_NEWADDR) {
            pal_err err = pal_net_if_add_ipv6_addr(netif, RTA_DATA(tb[IFA_ADDRESS]));
            if (err != PAL_ERR_OK) {
                HAPLogError(&logObject, "%s: add ipv6 address failed: %s", __func__, pal_err_string(err));
                break;
            }
            changed = true;
        } else {
            break;
        }
        if (changed) {
            pal_net_if_call_event_handlers(netif, PAL_NET_IF_EVENT_IPV6_ADDR_CHANGED);
        }
    } break;
    }
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
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
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
            case RTM_SETLINK:
                pal_net_if_handle_link_event(hdr);
                break;
            case RTM_NEWADDR:
            case RTM_DELADDR:
                pal_net_if_handle_addr_event(hdr);
                break;
            default:
                break;
            }
        }
    } while (loop);
}

static pal_err pal_net_if_get_ipv6_addrs() {
    int fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (fd == -1) {
        NET_IF_LOG_ERRNO(socket);
        return fd;
    }

    size_t reqlen = NLMSG_SPACE(sizeof(struct ifaddrmsg));
    char req[reqlen];
    memset(req, 0, reqlen);
    struct nlmsghdr *hdr = (struct nlmsghdr *)req;
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    hdr->nlmsg_type = RTM_GETADDR;
    struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(hdr);
    ifa->ifa_family = AF_INET6;

    pal_err err = PAL_ERR_OK;
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
            if (ifa->ifa_family != AF_INET6) {
                continue;
            }

            attr = IFA_RTA(ifa);
            attrlen = RTM_PAYLOAD(hdr);
            for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
                if (attr->rta_type == IFA_ADDRESS) {
                    pal_net_if *netif;
                    TAILQ_FOREACH(netif, &gstate.net_ifs, list_entry) {
                        if (netif->index == ifa->ifa_index) {
                            err = pal_net_if_add_ipv6_addr(netif,
                                (struct in6_addr *)RTA_DATA(attr));
                            if (err != PAL_ERR_OK) {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
    } while (1);

end:
    close(fd);
    return err;
}

static pal_err print_ipv6(pal_net_if *netif, pal_net_addr *addr, void *arg) {
    char buf[64];
    HAPLogDebug(&logObject, "      %s", pal_net_addr_get_string(addr, buf, sizeof(buf)));
    return PAL_ERR_OK;
}

static pal_err print_netif(pal_net_if *netif, void *arg) {
    char buf[256];
    pal_net_addr addr;
    bool loopback, up;
    pal_err err;
    pal_net_if_get_name(netif, buf);
    pal_net_if_is_loopback(netif, &loopback);
    pal_net_if_is_up(netif, &up);
    HAPLogDebug(&logObject, "  [%d] %s(%s, %s):", netif->index,
        buf, loopback ? "loopback" : "eth", up ? "up" : "down");
    uint8_t hwaddr[6];
    pal_net_if_get_hw_addr(netif, (uint8_t *)hwaddr);
    HAPLogDebug(&logObject, "    hw addr: %02X:%02X:%02X:%02X:%02X:%02X",
        hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
    err = pal_net_if_get_ipv4_addr(netif, &addr);
    if (err == PAL_ERR_OK) {
        HAPLogDebug(&logObject, "    ipv4 addr: %s\n",
            pal_net_addr_get_string(&addr, buf, sizeof(buf)));
    }
    if (!TAILQ_EMPTY(&netif->ipv6_addrs)) {
        HAPLogDebug(&logObject, "    ipv6 addrs:");
        pal_net_if_ipv6_addr_foreach(netif, print_ipv6, NULL);
    }
    return PAL_ERR_OK;
}

static void pal_net_if_init_ifs() {
    TAILQ_INIT(&gstate.net_ifs);

    struct if_nameindex *if_ni = if_nameindex();
    if (!if_ni) {
        return;
    }

    for (struct if_nameindex *i = if_ni;
        !(i->if_index == 0 && i->if_name == NULL); i++) {
        pal_net_if *netif = pal_net_if_create(i->if_index, i->if_name);
        HAPAssert(netif);
        TAILQ_INSERT_TAIL(&gstate.net_ifs, netif, list_entry);
    }

    if_freenameindex(if_ni);

    HAPError err = pal_net_if_get_ipv6_addrs();
    if (err) {
        HAPLogError(&logObject, "%s: failed to get ipv6 addresses: %s", __func__, pal_err_string(err));
        HAPFatalError();
    }

    HAPLogDebug(&logObject, "Network interfaces:");
    pal_net_if_foreach(print_netif, NULL);
}

static void pal_net_if_deinit_ifs() {
}

void pal_net_if_init() {
    HAPPrecondition(ginited == false);

    for (size_t i = 0; i < PAL_NET_IF_EVENT_COUNT; i++) {
        LIST_INIT(&gstate.event_cbs[i]);
    }

    gstate.event_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (gstate.event_fd == -1) {
        NET_IF_LOG_ERRNO(socket);
        HAPFatalError();
    }

    if (fcntl(gstate.event_fd, F_SETFL, fcntl(gstate.event_fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        NET_IF_LOG_ERRNO(fcntl);
        HAPFatalError();
    }

    struct  sockaddr_nl sa;
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

    pal_net_if_init_ifs();

    ginited = true;
}

void pal_net_if_deinit() {
    HAPPrecondition(ginited);

    pal_net_if_deinit_ifs();

    HAPPlatformFileHandleDeregister(gstate.event_handle);
    close(gstate.event_fd);

    ginited = false;
}

pal_net_if *pal_net_if_find(const char *name) {
    HAPPrecondition(name && name[0]);

    pal_net_if *netif;
    TAILQ_FOREACH(netif, &gstate.net_ifs, list_entry) {
        if (strcmp(name, netif->name)) {
            return netif;
        }
    }

    return NULL;
}

pal_err pal_net_if_foreach(pal_err (*func)(pal_net_if *netif, void *arg), void *arg) {
    HAPPrecondition(func);

    pal_net_if *netif;
    TAILQ_FOREACH(netif, &gstate.net_ifs, list_entry) {
        pal_err err = func(netif, arg);
        if (err != PAL_ERR_OK) {
            return err;
        }
    }

    return PAL_ERR_OK;
}

static bool pal_net_if_is_valid(pal_net_if *netif) {
    return netif->magic == PAL_NET_IF_MAGIC;
}

pal_err pal_net_if_is_up(pal_net_if *netif, bool *up) {
    HAPPrecondition(netif);
    HAPPrecondition(up);

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    *up = netif->up;

    return PAL_ERR_OK;
}

pal_err pal_net_if_is_loopback(pal_net_if *netif, bool *loopback) {
    HAPPrecondition(netif);
    HAPPrecondition(loopback);

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    *loopback = netif->loopback;

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_name(pal_net_if *netif, char name[PAL_NET_IF_NAME_MAX_LEN]) {
    HAPPrecondition(netif);
    HAPPrecondition(name);

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    strncpy(name, netif->name, PAL_NET_IF_NAME_MAX_LEN);

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_hw_addr(pal_net_if *netif, uint8_t addr[PAL_NET_IF_HW_ADDR_LEN]) {
    HAPPrecondition(netif);
    HAPPrecondition(addr);

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    memcpy(addr, netif->hw_addr, PAL_NET_IF_HW_ADDR_LEN);

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_ipv4_addr(pal_net_if *netif, pal_net_addr *_addr) {
    HAPPrecondition(netif);
    HAPPrecondition(_addr);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    if (addr->family != PAL_NET_ADDR_FAMILY_INET) {
        return PAL_ERR_NOT_FOUND;
    }

    *addr = netif->ipv4_addr;

    return PAL_ERR_OK;
}

pal_err pal_net_if_ipv6_addr_foreach(
        pal_net_if *netif,
        pal_err (*func)(pal_net_if *netif, pal_net_addr *addr, void *arg),
        void *arg) {
    HAPPrecondition(netif);
    HAPPrecondition(func);

    if (!pal_net_if_is_valid(netif)) {
        HAPLogError(&logObject, "%s: invalid netif", __func__);
        return PAL_ERR_INVALID_ARG;
    }

    struct pal_net_if_ipv6_addr *ipv6_addr;
    TAILQ_FOREACH(ipv6_addr, &netif->ipv6_addrs, list_entry) {
        pal_err err = func(netif, (pal_net_addr *)&ipv6_addr->addr, arg);
        if (err != PAL_ERR_OK) {
            return err;
        }
    }

    return PAL_ERR_OK;
}

pal_err pal_net_if_register_event_callback(pal_net_if *netif, pal_net_if_event event,
    pal_net_if_event_cb event_cb, void *arg) {
    HAPPrecondition(event >= PAL_NET_IF_EVENT_BEGIN && event <= PAL_NET_IF_EVENT_END);
    HAPPrecondition(event_cb);

    struct event_cb_desc *desc = pal_mem_alloc(sizeof(*desc));
    if (!desc) {
        HAPLogError(&logObject, "%s: failed to alloc event desc", __func__);
        return PAL_ERR_ALLOC;
    }

    desc->netif = netif;
    desc->cb = event_cb;
    desc->arg = arg;
    LIST_INSERT_HEAD(gstate.event_cbs + event, desc, list_entry);

    return PAL_ERR_OK;
}

pal_err pal_net_if_unregister_event_callback(pal_net_if *netif, pal_net_if_event event,
    pal_net_if_event_cb event_cb, void *arg) {
    HAPPrecondition(event >= PAL_NET_IF_EVENT_BEGIN && event <= PAL_NET_IF_EVENT_END);
    HAPPrecondition(event_cb);

    struct event_cb_desc *desc;
    LIST_FOREACH(desc, gstate.event_cbs + event, list_entry) {
        if (desc->netif == netif && desc->cb == event_cb) {
            LIST_REMOVE(desc, list_entry);
            pal_mem_free(desc);
            return PAL_ERR_OK;
        }
    }

    return PAL_ERR_INVALID_ARG;
}
