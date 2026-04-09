// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <pal/socket.h>
#include <pal/dns.h>
#include <pal/ssl.h>
#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPBase.h>
#include "app_int.h"
#include "lc.h"

#define LSTREAM_FRAME_LEN 1500
#define LSTREAM_LINE_LEN 256
#define LSTREAM_BUFFER_INITIAL_LEN LSTREAM_LINE_LEN
#define LSTREAM_BUFFER_RETAIN_LEN (LSTREAM_LINE_LEN * 2)
#define LSTREAM_BUFFER_SHRINK_THRESHOLD (LSTREAM_BUFFER_RETAIN_LEN * 4)
#define LSTREAM_CLIENT_NAME "StreamClient*"

HAP_ENUM_BEGIN(uint8_t, lstream_client_type) {
    LSTREAM_CLIENT_TCP,
    LSTREAM_CLIENT_TLS,
    LSTREAM_CLIENT_DTLS
} HAP_ENUM_END(uint8_t, lstream_client_type);

HAP_ENUM_BEGIN(uint8_t, lstream_client_state) {
    LSTREAM_CLIENT_NONE,
    LSTREAM_CLIENT_DNS_RESOLVING,
    LSTREAM_CLIENT_CONNECTING,
    LSTREAM_CLIENT_CONNECTED,
    LSTREAM_CLIENT_HANDSHAKING,
    LSTREAM_CLIENT_HANDSHAKED,
} HAP_ENUM_END(uint8_t, lstream_client_state);

const char *lstream_client_type_strs[] = {
    "TCP",
    "TLS",
    "DTLS",
    NULL,
};

typedef struct lstream_client {
    bool host_is_addr;
    bool sock_inited;
    lstream_client_state state;
    lstream_client_type type;
    uint16_t port;
    HAPPlatformTimerRef timer;
    lua_State *co;
    const char *host;
    pal_dns_req_ctx *dns_req;
    pal_ssl_ctx *sslctx;
    pal_socket_obj sock;
    lua_Alloc allocf;
    void *alloc_ud;
    char *buf;
    size_t buf_start;
    size_t buf_end;
    size_t buf_cap;
} lstream_client;

static const HAPLogObject lstream_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "stream",
};

static int lstream_client_async_read(lua_State *L, lstream_client *client, size_t maxlen, lua_KFunction k);

static void lstream_client_sslctx_free(lstream_client *client) {
    if (!client->sslctx) {
        return;
    }

    pal_ssl_ctx_deinit(client->sslctx);
    client->allocf(client->alloc_ud, client->sslctx, sizeof(*client->sslctx), 0);
    client->sslctx = NULL;
}

static size_t lstream_client_buffer_len(const lstream_client *client) {
    return client->buf_end - client->buf_start;
}

static const char *lstream_client_buffer_data(const lstream_client *client) {
    return client->buf ? client->buf + client->buf_start : "";
}

static void lstream_client_buffer_shrink(lstream_client *client, size_t cap) {
    if (!client->buf || client->buf_cap <= cap) {
        return;
    }

    char *buf = client->allocf(client->alloc_ud, client->buf, client->buf_cap, cap);
    if (buf) {
        client->buf = buf;
        client->buf_cap = cap;
    }
}

static void lstream_client_buffer_reset(lstream_client *client) {
    client->buf_start = 0;
    client->buf_end = 0;
    if (client->buf_cap > LSTREAM_BUFFER_SHRINK_THRESHOLD) {
        lstream_client_buffer_shrink(client, LSTREAM_BUFFER_RETAIN_LEN);
    }
}

static void lstream_client_buffer_free(lstream_client *client) {
    if (client->buf && client->allocf) {
        client->allocf(client->alloc_ud, client->buf, client->buf_cap, 0);
        client->buf = NULL;
    }
    client->buf_start = 0;
    client->buf_end = 0;
    client->buf_cap = 0;
}

static void lstream_client_buffer_compact(lstream_client *client) {
    size_t len = lstream_client_buffer_len(client);

    if (len == 0) {
        lstream_client_buffer_reset(client);
        return;
    }

    if (client->buf_start != 0) {
        memmove(client->buf, client->buf + client->buf_start, len);
        client->buf_start = 0;
        client->buf_end = len;
    }
}

static char *lstream_client_buffer_prep(lua_State *L, lstream_client *client, size_t len) {
    size_t unread = lstream_client_buffer_len(client);
    if (luai_unlikely(len > (~(size_t)0) - unread)) {
        luaL_error(L, "resulting string too large");
    }

    if (client->buf && client->buf_cap - client->buf_end >= len) {
        return client->buf + client->buf_end;
    }

    lstream_client_buffer_compact(client);
    if (client->buf && client->buf_cap - client->buf_end >= len) {
        return client->buf + client->buf_end;
    }

    size_t need = client->buf_end + len;
    size_t cap = client->buf_cap ? client->buf_cap : LSTREAM_BUFFER_INITIAL_LEN;
    while (cap < need) {
        size_t grown = cap + (cap >> 1);
        if (grown <= cap) {
            cap = need;
            break;
        }
        cap = grown;
    }

    char *buf = client->allocf(client->alloc_ud, client->buf, client->buf_cap, cap);
    if (luai_unlikely(!buf)) {
        luaL_error(L, "out of memory");
    }

    client->buf = buf;
    client->buf_cap = cap;
    return client->buf + client->buf_end;
}

static void lstream_client_buffer_addsize(lstream_client *client, size_t len) {
    client->buf_end += len;
}

static void lstream_client_buffer_consume(lstream_client *client, size_t len) {
    HAPPrecondition(len <= lstream_client_buffer_len(client));

    client->buf_start += len;
    if (client->buf_start == client->buf_end) {
        lstream_client_buffer_reset(client);
    }
}

static int lstream_client_buffer_push(lua_State *L, lstream_client *client, size_t len) {
    HAPPrecondition(len <= lstream_client_buffer_len(client));

    lua_pushlstring(L, lstream_client_buffer_data(client), len);
    lstream_client_buffer_consume(client, len);
    return 1;
}

static void lstream_client_cleanup(lstream_client *client) {
    client->state = LSTREAM_CLIENT_NONE;
    if (client->timer) {
        HAPPlatformTimerDeregister(client->timer);
        client->timer = 0;
    }
    if (client->dns_req) {
        pal_dns_cancel_request(client->dns_req);
        client->dns_req = NULL;
    }
    if (client->sock_inited) {
        pal_socket_obj_deinit(&client->sock);
        client->sock_inited = false;
    }
    lstream_client_sslctx_free(client);
    lstream_client_buffer_free(client);
}

static void lstream_client_create_finish(lstream_client *client, const char *errmsg) {
    HAPPrecondition(client);
    HAPPrecondition(client->co);

    if (client->timer) {
        HAPPlatformTimerDeregister(client->timer);
        client->timer = 0;
    }

    if (errmsg) {
        lstream_client_cleanup(client);
    }

    lua_State *co = client->co;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);
    if (errmsg) {
        lua_pushlightuserdata(co, (void *)errmsg);
    } else {
        lua_pushnil(co);
    }
    int status, nres;
    status = lc_resume(co, L, 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lstream_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void lstream_client_handshaked_cb(pal_socket_obj *o, pal_err err, void *arg) {
    lstream_client *client = arg;
    HAPAssert(&client->sock == o);

    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_HANDSHAKED;
        lstream_client_create_finish(client, NULL);
        break;
    default:
        lstream_client_create_finish(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_handshake(lstream_client *client) {
    HAPPrecondition(client);
    HAPPrecondition(client->state == LSTREAM_CLIENT_CONNECTED);

    if (client->type == LSTREAM_CLIENT_TCP) {
        lstream_client_create_finish(client, NULL);
        return;
    }

    pal_ssl_type ssltype;
    switch (client->type) {
    case LSTREAM_CLIENT_TLS:
        ssltype = PAL_SSL_TYPE_TLS;
        break;
    case LSTREAM_CLIENT_DTLS:
        ssltype = PAL_SSL_TYPE_DTLS;
        break;
    default:
        HAPFatalError();
    }

    HAPAssert(!client->sslctx);
    client->sslctx = client->allocf(client->alloc_ud, NULL, 0, sizeof(*client->sslctx));
    if (luai_unlikely(!client->sslctx)) {
        lstream_client_create_finish(client, "failed to create ssl context");
        return;
    }

    if (luai_unlikely(!pal_ssl_ctx_init(client->sslctx, ssltype, PAL_SSL_ENDPOINT_CLIENT,
        client->host_is_addr ? NULL : client->host, &client->sock, &(pal_ssl_bio_method) {
        .read = (void *)pal_socket_raw_recv,
        .write = (void *)pal_socket_raw_send,
    }))) {
        client->allocf(client->alloc_ud, client->sslctx, sizeof(*client->sslctx), 0);
        client->sslctx = NULL;
        lstream_client_create_finish(client, "failed to create ssl context");
        return;
    }
    pal_socket_set_bio(&client->sock, client->sslctx, &(pal_socket_bio_method) {
        .handshake = (void *)pal_ssl_handshake,
        .recv = (void *)pal_ssl_read,
        .send = (void *)pal_ssl_write,
        .pending = (void *)pal_ssl_pending,
    });

    pal_err err = pal_socket_handshake(&client->sock, lstream_client_handshaked_cb, client);
    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_HANDSHAKED;
        lstream_client_create_finish(client, NULL);
        break;
    case PAL_ERR_IN_PROGRESS:
        client->state = LSTREAM_CLIENT_HANDSHAKING;
        break;
    default:
        lstream_client_create_finish(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_connected_cb(pal_socket_obj *o, pal_err err, void *arg) {
    lstream_client *client = arg;
    HAPAssert(&client->sock == o);

    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_CONNECTED;
        lstream_client_handshake(client);
        break;
    default:
        lstream_client_create_finish(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_dns_response_cb(pal_err err, const char *addr,
    pal_net_addr_family af, void *arg) {
    lstream_client *client = arg;
    client->dns_req = NULL;

    switch (err) {
    case PAL_ERR_OK:
        break;
    case PAL_ERR_AGAIN:
        client->dns_req = pal_dns_start_request(client->host, PAL_NET_ADDR_FAMILY_UNSPEC,
            lstream_client_dns_response_cb, client);
        if (luai_unlikely(!client->dns_req)) {
            lstream_client_create_finish(client, "failed to start DNS resolution request");
        }
        return;
    default:
        lstream_client_create_finish(client, pal_err_string(err));
        return;
    }

    HAPAssert(addr);
    HAPAssert(af != PAL_NET_ADDR_FAMILY_UNSPEC);

    if (HAPStringAreEqual(addr, client->host)) {
        client->host_is_addr = true;
    }

    pal_socket_type socktype;
    switch (client->type) {
    case LSTREAM_CLIENT_TCP:
    case LSTREAM_CLIENT_TLS:
        socktype = PAL_SOCKET_TYPE_TCP;
        break;
    case LSTREAM_CLIENT_DTLS:
        socktype = PAL_SOCKET_TYPE_UDP;
        break;
    default:
        HAPFatalError();
    }

    if (luai_unlikely(!pal_socket_obj_init(&client->sock, socktype, af))) {
        lstream_client_create_finish(client, "failed to create socket object");
        return;
    }
    client->sock_inited = true;

    err = pal_socket_connect(&client->sock, addr,
        client->port, lstream_client_connected_cb, client);
    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_CONNECTED;
        lstream_client_handshake(client);
        break;
    case PAL_ERR_IN_PROGRESS:
        client->state = LSTREAM_CLIENT_CONNECTING;
        break;
    default:
        lstream_client_create_finish(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_timeout_timer_cb(HAPPlatformTimerRef timer, void *context) {
    lstream_client *client = context;
    client->timer = 0;
    lstream_client_create_finish(client, "timeout");
}

static int finishcreate(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    client->co = NULL;

    if (lua_islightuserdata(L, -1)) {
        lua_pushstring(L, lua_touserdata(L, -1));
        lua_error(L);
    }
    lua_pop(L, 1);
    return 1;
}

static int lstream_client_create(lua_State *L) {
    lstream_client_type type = luaL_checkoption(L, 1, NULL, lstream_client_type_strs);
    const char *host = luaL_checkstring(L, 2);
    lua_Integer port = luaL_checkinteger(L, 3);
    luaL_argcheck(L, (port >= 0) && (port <= 65535), 3, "port out of range");
    lua_Integer timeout = luaL_checkinteger(L, 4);
    luaL_argcheck(L, timeout >= 0, 4, "timeout out of range");

    lstream_client *client = lua_newuserdatauv(L, sizeof(*client), 1);
    memset(client, 0, sizeof(*client));
    luaL_setmetatable(L, LSTREAM_CLIENT_NAME);
    lua_pushvalue(L, 2);
    lua_setiuservalue(L, -2, 1);
    client->type = type;
    client->port = port;
    client->host = host;
    client->host_is_addr = false;
    client->sock_inited = false;
    client->co = NULL;
    client->dns_req = NULL;
    client->timer = 0;
    client->state = LSTREAM_CLIENT_NONE;
    client->allocf = lua_getallocf(L, &client->alloc_ud);

    if (luai_unlikely(HAPPlatformTimerRegister(&client->timer,
        HAPPlatformClockGetCurrent() + timeout,
        lstream_client_timeout_timer_cb, client) != kHAPError_None)) {
        luaL_error(L, "failed to create a timeout timer");
    }

    client->dns_req = pal_dns_start_request(host, PAL_NET_ADDR_FAMILY_UNSPEC,
        lstream_client_dns_response_cb, client);
    if (luai_unlikely(!client->dns_req)) {
        HAPPlatformTimerDeregister(client->timer);
        client->timer = 0;
        luaL_error(L, "failed to start DNS resolution request");
    }
    client->state = LSTREAM_CLIENT_DNS_RESOLVING;
    client->co = L;
    return lua_yieldk(L, 0, (lua_KContext)client, finishcreate);
}

static lstream_client *lstream_client_get(lua_State *L, int idx) {
    lstream_client *client = luaL_checkudata(L, idx, LSTREAM_CLIENT_NAME);
    if (luai_unlikely(client->state == LSTREAM_CLIENT_NONE)) {
        luaL_error(L, "attemp to use a destroyed client");
    }
    return client;
}

static int lstream_client_settimeout(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    lua_Integer ms = luaL_checkinteger(L, 2);
    luaL_argcheck(L, ms >= 0 && ms <= UINT32_MAX, 2, "ms out of range");

    pal_socket_set_timeout(&client->sock, ms);
    return 0;
}

static void lstream_client_write_sent_cb(pal_socket_obj *o, pal_err err, size_t sent_len, void *arg) {
    lstream_client *client = arg;
    HAPAssert(&client->sock == o);
    lua_State *co = client->co;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);
    lua_pushinteger(co, err);
    int status, nres;
    status = lc_resume(co, L, 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lstream_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finishwrite(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }
    return 0;
}

static int lstream_client_write(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    pal_err err;
    err = pal_socket_send(&client->sock, data, &len, true, lstream_client_write_sent_cb, client);
    switch (err) {
    case PAL_ERR_OK:
        return 0;
    case PAL_ERR_IN_PROGRESS:
        client->co = L;
        return lua_yieldk(L, 0, (lua_KContext)client, finishwrite);
    default:
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }
}

static void lstream_client_read_recved_cb(pal_socket_obj *o, pal_err err,
    const char *addr, uint16_t port, size_t len, void *arg) {
    lstream_client *client = arg;
    HAPAssert(&client->sock == o);

    lua_State *co = client->co;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);
    int narg = 1;
    if (err == PAL_ERR_OK) {
        narg = 2;
        lua_pushinteger(co, len);
    }
    lua_pushinteger(co, err);

    int status, nres;
    status = lc_resume(co, L, narg, &nres);  // stack <..., len, err>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lstream_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finishread(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (len == 0) {
        goto success;
    }

    lstream_client_buffer_addsize(client, len);

    size_t maxlen = lua_tointeger(L, 2);
    len = lstream_client_buffer_len(client);
    bool all = lua_toboolean(L, 3);
    if (len == maxlen || (!all && len != 0 && !pal_socket_readable(&client->sock))) {
        goto success;
    }

    return lstream_client_async_read(L, client, maxlen - len, finishread);

success:
    return lstream_client_buffer_push(L, client, lstream_client_buffer_len(client));
}

static int lstream_client_async_read(lua_State *L, lstream_client *client, size_t len, lua_KFunction k) {
    char *buf = lstream_client_buffer_prep(L, client, len);
    pal_err err = pal_socket_recv(&client->sock, buf, &len, lstream_client_read_recved_cb, client);
    if (err == PAL_ERR_IN_PROGRESS) {
        client->co = L;
        return lua_yieldk(L, 0, (lua_KContext)client, k);
    }

    int narg = 1;
    if (err == PAL_ERR_OK) {
        narg = 2;
        lua_pushinteger(L, len);
    }
    lua_pushinteger(L, err);
    return k(L, narg, (lua_KContext)client);
}

static int lstream_client_read(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    lua_Integer maxlen = luaL_checkinteger(L, 2);
    luaL_argcheck(L, maxlen > 0 && maxlen <= UINT32_MAX, 2, "maxlen out of range");
    if (lua_isnoneornil(L, 3)) {
        lua_pushboolean(L, false);
    }
    bool all = lua_toboolean(L, 3);

    size_t len = lstream_client_buffer_len(client);
    if (len >= (size_t)maxlen) {
        return lstream_client_buffer_push(L, client, maxlen);
    }
    if (!all && len > 0 && !pal_socket_readable(&client->sock)) {
        return lstream_client_buffer_push(L, client, len);
    }

    return lstream_client_async_read(L, client, maxlen - len, finishread);
}

static int finishreadall(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        if (err == PAL_ERR_TIMEOUT) {
            goto success;
        }
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (len == 0) {
        goto success;
    }

    lstream_client_buffer_addsize(client, len);
    return lstream_client_async_read(L, client, LSTREAM_FRAME_LEN, finishreadall);

success:
    return lstream_client_buffer_push(L, client, lstream_client_buffer_len(client));
}

static int lstream_client_readall(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    return lstream_client_async_read(L, client, LSTREAM_FRAME_LEN, finishreadall);
}

static const char *memfind(const char *s1, size_t l1, const char *s2, size_t l2) {
    if (l2 == 0) {
        return s1;  /* empty strings are everywhere */
    } else if (l2 > l1) {
        return NULL;  /* avoids a negative 'l1' */
    } else {
        const char *init;  /* to search for a '*s2' inside 's1' */
        l2--;  /* 1st char will be checked by 'memchr' */
        l1 = l1 - l2;  /* 's2' cannot be found after that */
        while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
            init++;   /* 1st char is already checked */
            if (memcmp(init, s2 + 1, l2) == 0) {
                return init - 1;
            } else {  /* correct 'l1' and 's1' to try again */
                l1 -= init - s1;
                s1 = init;
            }
        }
        return NULL;  /* not found */
    }
}

static bool lstream_client_getline(lua_State *L, lstream_client *client, bool skip,
    const char *sep, size_t seplen, size_t init) {
    size_t len = lstream_client_buffer_len(client);
    const char *data = lstream_client_buffer_data(client);
    const char *s = memfind(data + init, len - init, sep, seplen);
    if (s) {
        size_t linelen = (size_t)(s - data);
        lua_pushlstring(L, data, skip ? linelen : linelen + seplen);
        lstream_client_buffer_consume(client, linelen + seplen);
        return true;
    }
    return false;
}

static int finishreadline(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (len == 0) {
        lua_pushstring(L, "read EOF");
        return lua_error(L);
    }

    size_t seplen;
    const char *sep = luaL_optlstring(L, 2, "\n", &seplen);
    bool skip = lua_toboolean(L, 3);
    size_t init = 0;
    size_t unread = lstream_client_buffer_len(client);
    if (seplen < unread) {
        init = unread + 1 - seplen;
    }

    lstream_client_buffer_addsize(client, len);

    if (lstream_client_getline(L, client, skip, sep, seplen, init)) {
        return 1;
    }

    return lstream_client_async_read(L, client, LSTREAM_LINE_LEN, finishreadline);
}

static int lstream_client_readline(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    size_t seplen;
    const char *sep = luaL_optlstring(L, 2, "\n", &seplen);
    bool skip = lua_toboolean(L, 3);

    if (lstream_client_getline(L, client, skip, sep, seplen, 0)) {
        return 1;
    }

    return lstream_client_async_read(L, client, LSTREAM_LINE_LEN, finishreadline);
}

static int lstream_client_close(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    lstream_client_cleanup(client);
    return 0;
}

static int lstream_client_gc(lua_State *L) {
    lstream_client *client = luaL_checkudata(L, 1, LSTREAM_CLIENT_NAME);
    lstream_client_cleanup(client);
    return 0;
}

static int lstream_client_tostring(lua_State *L) {
    lstream_client *client = luaL_checkudata(L, 1, LSTREAM_CLIENT_NAME);
    if (client) {
        lua_pushfstring(L, "stream client (%p)", client);
    } else {
        lua_pushliteral(L, "stream client (destroyed)");
    }
    return 0;
}

static const luaL_Reg lstream_funcs[] = {
    {"client", lstream_client_create},
    {NULL, NULL},
};

/*
 * metamethods for stream client
 */
static const luaL_Reg lstream_client_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lstream_client_gc},
    {"__close", lstream_client_gc},
    {"__tostring", lstream_client_tostring},
    {NULL, NULL}
};

/*
 * methods for stream client
 */
static const luaL_Reg lstream_client_meth[] = {
    {"settimeout", lstream_client_settimeout},
    {"write", lstream_client_write},
    {"read", lstream_client_read},
    {"readall", lstream_client_readall},
    {"readline", lstream_client_readline},
    {"close", lstream_client_close},
    {NULL, NULL},
};

static void lstream_createmeta(lua_State *L) {
    luaL_newmetatable(L, LSTREAM_CLIENT_NAME);  /* metatable for stream client */
    luaL_setfuncs(L, lstream_client_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lstream_client_meth);  /* create method table */
    luaL_setfuncs(L, lstream_client_meth, 0);  /* add stream client methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_stream(lua_State *L) {
    luaL_newlib(L, lstream_funcs);
    lstream_createmeta(L);
    return 1;
}
