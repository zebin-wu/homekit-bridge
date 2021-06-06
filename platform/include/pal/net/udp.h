// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_UDP_H_
#define PLATFORM_INCLUDE_PAL_NET_UDP_H_

#include <stdint.h>
#include <stddef.h>
#include <pal/net/types.h>

/**
 * Network UDP interfaces.
 * Every platform must to implement it.
 */

/**
 * Opaque structure for the UDP protocol control block (PCB).
 */
typedef struct pal_net_udp pal_net_udp;

/**
 * Allocate a UDP PCB.
 *
 * @param domain Specified communication domain.
 * @return UDP PCB or NULL if the allocation fails.
 */
pal_net_udp *pal_net_udp_new(pal_net_domain domain);

/**
 * Bind a UDP PCB to a local IP address and port.
 *
 * @param udp UDP PCB.
 * @param addr Local address to use, or NULL if any address can be used.
 * @param port Local UDP port number, in host order.
 * @return zero on success, error number on error.
 */
pal_net_err pal_net_udp_bind(pal_net_udp *udp, const char *addr, uint16_t port);

/**
 * Set the remote address and/or port for a UDP PCB.
 *
 * This sets the destination address for subsequent pal_net_udp_send() calls.
 *
 * @param udp UDP PCB.
 * @param addr Remote address to use.
 * @param port Remote UDP port number, in host order.
 * @return zero on success, error number on error.
 */
pal_net_err pal_net_udp_connect(pal_net_udp *udp, const char *addr, uint16_t port);

/**
 * Send a packet on the UDP PCB.
 *
 * @param udp UDP PCB.
 * @param buf A pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @return zero on success, error number on error.
 */
pal_net_err pal_net_udp_send(pal_net_udp *udp, const void *data, size_t len);

/**
 * A callback called when a packet is received on the UDP PCB.
 *
 * @param udp UDP PCB which reports the received data.
 * @param data UDP packet received.
 * @param len Legnth of the received data.
 * @param from_ip Source IP address that the packet comes from.
 * @param from_port Source port, in host order.
 * @param arg The last paramter of pal_net_udp_set_recv_cb().
 */
typedef void (*pal_net_udp_recv_cb)(pal_net_udp *udp, void *data, size_t len,
    const char *from_addr, uint16_t from_port, void *arg);

/**
 * Set the receive callback for a UDP PCB.
 *
 * @param udp UDP PCB.
 * @param recv_cb A callback called when a packet is received on the UDP PCB.
 * @param recv_arg The value to be passed as the last argument to recv_cb.
 */
void pal_net_udp_set_recv_cb(pal_net_udp *udp, pal_net_udp_recv_cb cb, void *recv_arg);

/**
 * Free a UDP PCB.
 *
 * @param udp UDP PCB.
 */
void pal_net_udp_free(pal_net_udp *udp);

#endif  // PLATFORM_INCLUDE_PAL_NET_UDP_H_
