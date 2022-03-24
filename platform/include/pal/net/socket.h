// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_SOCKET_H_
#define PLATFORM_INCLUDE_PAL_NET_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pal/err.h>
#include <pal/net/addr.h>

/**
 * Communication type.
 */
typedef enum {
    PAL_SOCKET_TYPE_TCP,            /**< TCP */
    PAL_SOCKET_TYPE_UDP,            /**< UDP */
} pal_socket_type;

/**
 * Opaque structure for the socket object.
 */
typedef struct pal_socket_obj pal_socket_obj;

/**
 * Create a socket object.
 *
 * @param type Specified communication type.
 * @param af Specified address family.
 * @returns a socket pointer or NULL if the allocation fails.
 */
pal_socket_obj *pal_socket_create(pal_socket_type type, pal_addr_family af);

/**
 * Destroy the socket object.
 *
 * @param o The pointer to the socket object.
 */
void pal_socket_destroy(pal_socket_obj *o);

/**
 * Set the timeout.
 */
void pal_socket_set_timeout(pal_socket_obj *o, uint32_t ms);

/**
 * Enable broadcast.
 *
 * @param o The pointer to the socket object.
 * @returns zero on success, error number on error.
 */
pal_err pal_socket_enable_broadcast(pal_socket_obj *o);

/**
 * Bind a local IP address and port.
 *
 * @param o The pointer to the socket object.
 * @param addr Local address to use.
 * @param port Local port number, in host order.
 * @returns zero on success, error number on error.
 */
pal_err pal_socket_bind(pal_socket_obj *o, const char *addr, uint16_t port);

/**
 * Listen for connections.
 * 
 * @param o The pointer to the socket object.
 * @param backlog The maximum length to which the queue of pending connections.
 * @returns zero on success, error number on error.
 */
pal_err pal_socket_listen(pal_socket_obj *o, int backlog);

/**
 * A callback called when the socket accepted a new connection.
 *
 * @param o The pointer to the socket object.
 * @param err The error of the accept procress.
 * @param new_o The pointer to the new socket object for the incoming connection.
 * @param addr The remote address.
 * @param port The remote port.
 * @param arg The last paramter of pal_socket_accept().
 */
typedef void (*pal_socket_accepted_cb)(pal_socket_obj *o, pal_err err,
    pal_socket_obj *new_o, const char *addr, uint16_t port, void *arg);

/**
 * Accept a connection.
 *
 * @param o The pointer to the socket object.
 * @param new_o The pointer to the new socket object for the incoming connection.
 * @param addr The buffer for storing the remote address.
 * @param addrlen The length of the buffer.
 * @param accepted_cb A callback called when the socket accepted a new connection.
 * @param arg The value to be passed as the last argument to accept_cb.
 * @return PAL_ERR_OK on success
 * @return PAL_ERR_IN_PROGRESS means it will take a while to accept,
 *         @p accepted_cb will be called when accepting the connection.
 * @return other error number on failure.
 */
pal_err pal_socket_accept(pal_socket_obj *o, pal_socket_obj **new_o, char *addr, size_t addrlen, uint16_t *port,
    pal_socket_accepted_cb accepted_cb, void *arg);

/**
 * A callback called when the connection is done.
 *
 * @param o The pointer to the socket object.
 * @param err The error of the connection.
 * @param arg The last paramter of pal_socket_connect().
 */
typedef void (*pal_socket_connected_cb)(pal_socket_obj *o, pal_err err, void *arg);

/**
 * Initiate a connection.
 *
 * @param o The pointer to the socket object.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @param connected_cb A callback called when the connection is done.
 * @param arg The value to be passed as the last argument to connected_cb.
 * @return PAL_ERR_OK on success
 * @return PAL_ERR_IN_PROGRESS means it will take a while to connect,
 *         @p connected_cb will be called when the connection is established.
 * @return other error number on failure.
 */
pal_err pal_socket_connect(pal_socket_obj *o, const char *addr, uint16_t port,
    pal_socket_connected_cb connected_cb, void *arg);

/**
 * A callback called when the data is sent.
 *
 * @param o The pointer to the socket object.
 * @param err The error of the send procress.
 * @param arg The last paramter of pal_socket_send() or pal_socket_sendto().
 */
typedef void (*pal_socket_sent_cb)(pal_socket_obj *o, pal_err err, size_t sent_len, void *arg);

/**
 * Send data.
 *
 * @param o The pointer to the socket object.
 * @param data A pointer to the data to be sent.
 * @param len Length of @p data.
 * @param all Whether @p data is completely sent.
 * @param sent_cb A callback called when @p data is sent.
 * @param arg The value to be passed as the last argument to @p sent_cb.
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_IN_PROGRESS means it will take a while to send,
 *         @p sent_cb will be called when @p data is sent.
 * @return other error number on failure.
 */
pal_err pal_socket_send(pal_socket_obj *o, const void *data, size_t *len, bool all,
    pal_socket_sent_cb sent_cb, void *arg);

/**
 * Send data to remote addr and port.
 *
 * @param o The pointer to the socket object.
 * @param data A pointer to the data to be sent.
 * @param len Length of @p data.
 * @param addr Remote address to use.
 * @param port Remote port number, in host order.
 * @param all Whether @p data is completely sent.
 * @param sent_cb A callback called when the data is sent.
 * @param arg The value to be passed as the last argument to @p sent_cb.
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_IN_PROGRESS means it will take a while to send,
 *         @p sent_cb will be called when @p data is sent.
 * @return other error number on failure.
 */
pal_err pal_socket_sendto(pal_socket_obj *o, const void *data, size_t *len,
    const char *addr, uint16_t port, bool all, pal_socket_sent_cb sent_cb, void *arg);

/**
 * A callback called when a socket received data.
 *
 * @param o The pointer to the socket object.
 * @param err The error of the receive procress.
 * @param addr The remote address.
 * @param port The remote port.
 * @param data The received data.
 * @param len The length of the received data.
 * @param arg The last paramter of pal_socket_recv().
 */
typedef void (*pal_socket_recved_cb)(pal_socket_obj *o, pal_err err,
    const char *addr, uint16_t port, void *data, size_t len, void *arg);

/**
 * Receive data.
 *
 * @param o The pointer to the socket object.
 * @param maxlen The max length of data you want to received.
 * @param recved_cb A callback called when a socket received data.
 * @param arg The value to be passed as the last argument to recved_cb.
 * @return PAL_ERR_IN_PROGRESS means it will take a while to recv,
 *         @p recved_cb will be called when the data is received.
 * @return other error number on failure.
 */
pal_err pal_socket_recv(pal_socket_obj *o, size_t maxlen, pal_socket_recved_cb recved_cb, void *arg);

/**
 * Whether the socket is readable.
 *
 * @param o The pointer to the socket object.
 * @returns true if there is buffered record data in the socket and false otherwise.
 */
bool pal_socket_readable(pal_socket_obj *o);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NET_SOCKET_H_
