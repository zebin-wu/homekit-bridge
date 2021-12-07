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
} pal_socket_type;

/**
 * Communication domain.
 */
typedef enum {
    PAL_SOCKET_DOMAIN_IPV4,         /**< IPv4 Internet protocols. */
    PAL_SOCKET_DOMAIN_IPV6,         /**< IPv6 Internet protocols. */
} pal_socket_domain;

/**
 * Socket error numbers.
 */
typedef enum {
    PAL_SOCKET_ERR_OK,              /**< No error. */
    PAL_SOCKET_TIMEOUT,             /**< Timeout. */
    PAL_SOCKET_ERR_IN_PROGRESS,     /**< In progress. */
    PAL_SOCKET_ERR_UNKNOWN,         /**< Unknown. */
    PAL_SOCKET_ERR_ALLOC,           /**< Failed to alloc. */
    PAL_SOCKET_ERR_INVALID_ARG,     /**< Invalid argument. */
    PAL_SOCKET_ERR_INVALID_STATE,   /**< Invalid state. */
    PAL_SOCKET_ERR_NOT_CONN,        /**< Not connected. */
} pal_socket_err;

/**
 * Opaque structure for the socket object.
 */
typedef struct pal_socket_obj pal_socket_obj;

/**
 * Create a socket object.
 *
 * @param type Specified communication type.
 * @param domain Specified communication domain.
 * @returns a socket pointer or NULL if the allocation fails.
 */
pal_socket_obj *pal_socket_create(pal_socket_type type, pal_socket_domain domain);

/**
 * Destroy the socket object.
 *
 * @param socket The pointer to the socket object.
 */
void pal_socket_destroy(pal_socket_obj *o);

/**
 * Set the timeout.
 */
void pal_socket_set_timeout(uint32_t ms);

/**
 * Bind a local IP address and port.
 *
 * @param socket The pointer to the socket object.
 * @param addr Local address to use.
 * @param port Local port number, in host order.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_bind(pal_socket_obj *o, const char *addr, uint16_t port);

/**
 * Listen for connections.
 * 
 * @param socket The pointer to the socket object.
 * @param backlog The maximum length to which the queue of pending connections.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_listen(pal_socket_obj *o, int backlog);

/**
 * A callback called when the socket accepted a new connection.
 *
 * @param new_o The pointer to the new socket object for the incoming connection.
 * @param arg The last paramter of pal_socket_accept().
 */
typedef void (*pal_socket_accepted_cb)(pal_socket_obj *new_o, void *arg);

/**
 * Accept a connection.
 *
 * @param socket The pointer to the socket object.
 * @param accepted_cb A callback called when the socket accepted a new connection.
 * @param arg The value to be passed as the last argument to accept_cb.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_accept(pal_socket_obj *o, pal_socket_accepted_cb accepted_cb, void *arg);

/**
 * A callback called when the connection is done.
 *
 * @param socket The pointer to the socket object.
 * @param err The error of the connection.
 * @param arg The last paramter of pal_socket_connect().
 */
typedef void (*pal_socket_connected_cb)(pal_socket_obj *o, pal_socket_err err, void *arg);

/**
 * Initiate a connection.
 *
 * @param socket The pointer to the socket object.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @param connected_cb A callback called when the connection is done.
 * @param arg The value to be passed as the last argument to connected_cb.
 * @returns zero on success, error number on error.
 * @return PAL_SOCKET_ERR_OK on success
 * @return PAL_SOCKET_ERR_IN_PROGRESS means it will take a while to connect,
 *     the connected_cb will be called when the connection is done.
 */
pal_socket_err pal_socket_connect(pal_socket_obj *o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg);

/**
 * A callback called when the message is sent.
 *
 * @param socket The pointer to the socket object.
 * @param err The error of the send procress.
 * @param arg The last paramter of pal_socket_send() or pal_socket_sendto().
 */
typedef void (*pal_socket_sent_cb)(pal_socket_obj *o, pal_socket_err err, void *arg);

/**
 * Send a message.
 *
 * @param socket The pointer to the socket object.
 * @param buf A pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @param sent_cb A callback called when the message is sent.
 * @param arg The value to be passed as the last argument to sent_cb.
 * @returns zero on success, error number on error.
 * @return PAL_SOCKET_ERR_OK on success
 * @return PAL_SOCKET_ERR_IN_PROGRESS means it will take a while to send, the sent_cb will be called when the message is sent.
 */
pal_socket_err pal_socket_send(pal_socket_obj *o, const void *data, size_t len, pal_socket_sent_cb sent_cb, void *arg);

/**
 * Send a message to remote addr and port.
 *
 * @param socket The pointer to the socket object.
 * @param buf A pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @param sent_cb A callback called when the message is sent.
 * @param arg The value to be passed as the last argument to sent_cb.
 * @returns zero on success, error number on error.
 * @return PAL_SOCKET_ERR_OK on success
 * @return PAL_SOCKET_ERR_IN_PROGRESS means it will take a while to send, the sent_cb will be called when the message is sent.
 */
pal_socket_err pal_socket_sendto(pal_socket_obj *o, const void *data, size_t len,
    const char *addr, uint16_t port, pal_socket_sent_cb sent_cb, void *arg);

/**
 * A callback called when a socket received a message.
 *
 * @param socket The pointer to the socket object.
 * @param err The error of the receive procress.
 * @param addr The remote address.
 * @param port The remote port.
 * @param arg The last paramter of pal_socket_recv().
 */
typedef void (*pal_socket_recved_cb)(pal_socket_obj *o, pal_socket_err err,
    const char *addr, uint16_t port, void *data, size_t len, void *arg);

/**
 * Receive a message.
 *
 * @param socket The pointer to the socket object.
 * @param maxlen The max length of the message.
 * @param recved_cb A callback called when a socket received a message.
 * @param arg The value to be passed as the last argument to recved_cb.
 * @returns zero on success, error number on error.
 */
pal_socket_err pal_socket_recv(pal_socket_obj *o, size_t maxlen, pal_socket_recved_cb recved_cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_SOCKET_H_