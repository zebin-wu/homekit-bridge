// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/err.h>
#include <pal/net_addr.h>
#include <pal/net_addr_int.h>

pal_err pal_net_addr_init(pal_net_addr *_addr, pal_net_addr_family af, const char *s) {
    HAPPrecondition(_addr);
    HAPPrecondition(af == PAL_NET_ADDR_FAMILY_INET || af == PAL_NET_ADDR_FAMILY_INET6);
    HAPPrecondition(s);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    int ret = inet_pton(af == PAL_NET_ADDR_FAMILY_INET ? AF_INET : AF_INET6, s, &addr->data);
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

    return ((pal_net_addr_int *)addr)->af;
}

const char *pal_net_addr_get_string(pal_net_addr *_addr, char *buf, size_t buflen) {
    HAPPrecondition(_addr);
    HAPPrecondition(buf);
    HAPPrecondition(buflen > 0);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    switch (addr->af) {
    case PAL_NET_ADDR_FAMILY_INET:
        return inet_ntop(AF_INET, &addr->data, buf, buflen);
    case PAL_NET_ADDR_FAMILY_INET6:
        return inet_ntop(AF_INET6, &addr->data, buf, buflen);
    default:
        HAPFatalError();
    }
}
