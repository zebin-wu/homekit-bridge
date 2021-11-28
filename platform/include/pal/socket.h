// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_SOCKET_H_
#define PLATFORM_INCLUDE_PAL_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * Communication type.
 */
typedef enum {
    PAL_SOCKET_TYPE_TCP,            /**< TCP */
    PAL_SOCKET_TYPE_UDP,            /**< UDP */

    PAL_SOCKET_TYPE_COUNT,          /**< Count of enums. */
} pal_socket_type;

/**
 * Communication domain.
 */
typedef enum {
    PAL_SOCKET_DOMAIN_IPV4,         /**< IPv4 Internet protocols. */
    PAL_SOCKET_DOMAIN_IPV6,         /**< IPv6 Internet protocols. */

    PAL_SOCKET_DOMAIN_COUNT,        /**< Count of enums. */
} pal_socket_domain;

/**
 * Socket error numbers.
 */
typedef enum {
    PAL_SOCKET_ERR_OK,              /**< No error. */
    PAL_SOCKET_ERR_IN_PROGRESS,     /**< In progress. */
    PAL_SOCKET_ERR_UNKNOWN,         /**< Unknown. */
    PAL_SOCKET_ERR_ALLOC,           /**< Failed to alloc. */
    PAL_SOCKET_ERR_INVALID_ARG,     /**< Invalid argument. */
    PAL_SOCKET_ERR_NOT_CONN,        /**< Not connected. */
} pal_socket_err;

/**
 * Opaque structure for the socket.
 */
typedef struct pal_socket pal_socket;

/**
 * Create a socket.
 *
 * @param type Specified communication type.
 * @param domain Specified communication domain.
 * @returns a socket pointer or NULL if the allocation fails.
 */
pal_socket pal_socket_create(pal_socket_type type, pal_socket_domain domain);

/**
 * Destroy the socket.
 *
 * @param udp The pointer to the socket.
 */
void pal_socket_destroy(pal_socket *socket);

/**
 * Bind a local IP address and port.
 *
 * @param socket The pointer to the socket.
 * @param addr Local address to use.
 * @param port Local port number, in host order.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_bind(pal_socket *socket, const char *addr, uint16_t port);

/**
 * A callback called when the connection is done.
 *
 * @param socket The pointer to the socket.
 * @param err The error of the connection.
 * @param arg The last paramter of pal_socket_connect().
 */
typedef void (pal_socket_conn_done_cb)(pal_socket *socket, pal_socket_err *err, void *arg);

/**
 * Set the remote address and port.
 *
 * @param socket The pointer to the socket.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @param done_cb The callback call when the connection is done.
 * @param arg The value to be passed as the last argument to done_cb.
 *
 * @return PAL_SOCKET_ERR_OK on success
 * @return PAL_SOCKET_ERR_IN_PROGRESS means it will take a while to connect, the done_cb will be called when the connection is done.
 */
pal_socket_err pal_socket_connect(pal_socket *socket, const char *addr, uint16_t port, pal_socket_conn_done_cb done_cb, void *arg);

/**
 * Send a packet.
 *
 * @attention This function is a non-blocking function, the packet will
 * be put in the queue first and sent out when the PCB is writable.
 *
 * @param socket The pointer to the socket.
 * @param buf A pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_send(pal_socket *socket, const void *data, size_t len);

/**
 * Send a packet to remote addr and port.
 *
 * @attention This function is a non-blocking function, the packet will
 * be put in the queue first and sent out when the PCB is writable.
 *
 * @param socket The pointer to the socket.
 * @param buf A pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_sendto(pal_socket *socket, const void *data, size_t len,
    const char *addr, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_SOCKET_H_
