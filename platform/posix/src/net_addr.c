// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <sys/socket.h>
#include <pal/err.h>
#include <pal/net_addr.h>
#include <pal/net_addr_int.h>

pal_err pal_net_addr_init(pal_net_addr *_addr, pal_net_addr_family family, const char *s) {
    HAPPrecondition(_addr);
    HAPPrecondition(family == PAL_NET_ADDR_FAMILY_INET || family == PAL_NET_ADDR_FAMILY_INET6);
    HAPPrecondition(s);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    addr->family = family;
    int ret = inet_pton(family == PAL_NET_ADDR_FAMILY_INET ? AF_INET : AF_INET6, s, &addr->u);
    switch (ret) {
    case 1:
        return PAL_ERR_OK;
    case 0:
        return PAL_ERR_INVALID_ARG;
    default:
        /* If af does not contain a valid address family, -1 is returned. */
        HAPFatalError();
    }
}

pal_net_addr_family pal_net_addr_get_family(pal_net_addr *addr) {
    HAPPrecondition(addr);

    return ((pal_net_addr_int *)addr)->family;
}

const char *pal_net_addr_get_string(pal_net_addr *_addr, char *buf, size_t buflen) {
    HAPPrecondition(_addr);
    HAPPrecondition(buf);
    HAPPrecondition(buflen > 0);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    switch (addr->family) {
    case PAL_NET_ADDR_FAMILY_INET:
        return inet_ntop(AF_INET, &addr->u, buf, buflen);
    case PAL_NET_ADDR_FAMILY_INET6:
        return inet_ntop(AF_INET6, &addr->u, buf, buflen);
    default:
        HAPFatalError();
    }
}
