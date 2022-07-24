// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <pal/net/socket.h>
#include <pal/net/dns.h>
#include <pal/crypto/ssl.h>
#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPBase.h>
#include "app_int.h"
#include "lc.h"

#define LSTREAM_FRAME_LEN 1500
#define LSTREAM_LINE_LEN 256
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
    bool sslctx_pending;
    bool sslctx_inited;
    lstream_client_state state;
    lstream_client_type type;
    uint16_t port;
    HAPPlatformTimerRef timer;
    lua_State *co;
    const char *host;
    pal_dns_req_ctx *dns_req;
    pal_ssl_ctx sslctx;
    pal_socket_obj *sock;
    luaL_Buffer B;
} lstream_client;

static const HAPLogObject lstream_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lstream",
};

static int lstream_client_async_read(lua_State *L, lstream_client *client, size_t maxlen, lua_KFunction k);

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
    if (client->sock) {
        pal_socket_destroy(client->sock);
        client->sock = NULL;
    }
    if (client->sslctx_inited) {
        pal_ssl_ctx_deinit(&client->sslctx);
        client->sslctx_inited = false;
    }
}

static void lstream_client_create_finsh(lstream_client *client, const char *errmsg) {
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
    HAPAssert(client->sock == o);

    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_HANDSHAKED;
        lstream_client_create_finsh(client, NULL);
        break;
    default:
        lstream_client_create_finsh(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_handshake(lstream_client *client) {
    HAPPrecondition(client);
    HAPPrecondition(client->state == LSTREAM_CLIENT_CONNECTED);

    if (client->type == LSTREAM_CLIENT_TCP) {
        lstream_client_create_finsh(client, NULL);
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

    HAPAssert(!client->sslctx_inited);
    if (luai_unlikely(!pal_ssl_ctx_init(&client->sslctx, ssltype, PAL_SSL_ENDPOINT_CLIENT,
        client->host_is_addr ? NULL : client->host, client->sock, &(pal_ssl_bio_method) {
        .read = (void *)pal_socket_raw_recv,
        .write = (void *)pal_socket_raw_send,
    }))) {
        lstream_client_create_finsh(client, "failed to create ssl context");
        return;
    }
    pal_socket_set_bio(client->sock, &client->sslctx, &(pal_socket_bio_method) {
        .handshake = (void *)pal_ssl_handshake,
        .recv = (void *)pal_ssl_read,
        .send = (void *)pal_ssl_write,
        .pending = (void *)pal_ssl_pending,
    });
    client->sslctx_inited = true;

    pal_err err = pal_socket_handshake(client->sock, lstream_client_handshaked_cb, client);
    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_HANDSHAKED;
        lstream_client_create_finsh(client, NULL);
        break;
    case PAL_ERR_IN_PROGRESS:
        client->state = LSTREAM_CLIENT_HANDSHAKING;
        break;
    default:
        lstream_client_create_finsh(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_connected_cb(pal_socket_obj *o, pal_err err, void *arg) {
    lstream_client *client = arg;
    HAPAssert(client->sock == o);

    switch (err) {
    case PAL_ERR_OK:
        client->state = LSTREAM_CLIENT_CONNECTED;
        lstream_client_handshake(client);
        break;
    default:
        lstream_client_create_finsh(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_dns_response_cb(const char *errmsg, const char *addr,
    pal_addr_family af, void *arg) {
    lstream_client *client = arg;
    client->dns_req = NULL;

    if (errmsg) {
        lstream_client_create_finsh(client, errmsg);
        return;
    }

    HAPAssert(addr);
    HAPAssert(af != PAL_ADDR_FAMILY_UNSPEC);

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

    client->sock = pal_socket_create(socktype, af);
    if (luai_unlikely(!client->sock)) {
        lstream_client_create_finsh(client, "failed to create socket object");
        return;
    }

    pal_err err = pal_socket_connect(client->sock, addr,
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
        lstream_client_create_finsh(client, pal_err_string(err));
        break;
    }
}

static void lstream_client_timeout_timer_cb(HAPPlatformTimerRef timer, void *context) {
    lstream_client *client = context;
    client->timer = 0;
    lstream_client_create_finsh(client, "timeout");
}

static int finshcreate(lua_State *L, int status, lua_KContext extra) {
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

    lstream_client *client = lua_newuserdatauv(L, sizeof(*client), 2);
    luaL_setmetatable(L, LSTREAM_CLIENT_NAME);
    lua_pushvalue(L, 2);
    lua_setiuservalue(L, -2, 1);
    lua_pushstring(L, "");
    lua_setiuservalue(L, -2, 2);
    client->type = type;
    client->port = port;
    client->host = host;
    client->host_is_addr = false;
    client->sslctx_pending = false;
    client->sslctx_inited = false;
    client->sock = NULL;
    client->co = NULL;
    client->dns_req = NULL;
    client->timer = 0;
    client->state = LSTREAM_CLIENT_NONE;

    if (luai_unlikely(HAPPlatformTimerRegister(&client->timer,
        HAPPlatformClockGetCurrent() + timeout,
        lstream_client_timeout_timer_cb, client) != kHAPError_None)) {
        luaL_error(L, "failed to create a timeout timer");
    }

    client->dns_req = pal_dns_start_request(host, PAL_ADDR_FAMILY_UNSPEC,
        lstream_client_dns_response_cb, client);
    if (luai_unlikely(!client->dns_req)) {
        HAPPlatformTimerDeregister(client->timer);
        client->timer = 0;
        luaL_error(L, "failed to start DNS resolution request");
    }
    client->state = LSTREAM_CLIENT_DNS_RESOLVING;
    client->co = L;
    return lua_yieldk(L, 0, (lua_KContext)client, finshcreate);
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

    pal_socket_set_timeout(client->sock, ms);
    return 0;
}

static void lstream_client_write_sent_cb(pal_socket_obj *o, pal_err err, size_t sent_len, void *arg) {
    lstream_client *client = arg;
    HAPAssert(client->sock == o);
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

static int finshwrite(lua_State *L, int status, lua_KContext extra) {
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
    err = pal_socket_send(client->sock, data, &len, true, lstream_client_write_sent_cb, client);
    switch (err) {
    case PAL_ERR_OK:
        return 0;
    case PAL_ERR_IN_PROGRESS:
        client->co = L;
        return lua_yieldk(L, 0, (lua_KContext)client, finshwrite);
    default:
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }
}

static void lstream_client_read_recved_cb(pal_socket_obj *o, pal_err err,
    const char *addr, uint16_t port, void *data, size_t len, void *arg) {
    lstream_client *client = arg;
    HAPAssert(client->sock == o);

    lua_State *co = client->co;
    lua_State *L = lc_getmainthread(co);

    HAPAssert(lua_gettop(L) == 0);
    int narg = 1;
    if (err == PAL_ERR_OK) {
        narg = 3;
        lua_pushlightuserdata(co, data);
        lua_pushinteger(co, len);
    }
    lua_pushinteger(co, err);

    int status, nres;
    status = lc_resume(co, L, narg, &nres);  // stack <..., data, len, err>
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        HAPLogError(&lstream_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int finshread(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    luaL_Buffer *B = &client->B;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        luaL_pushresult(B);
        lua_setiuservalue(L, 1, 2);
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    void *data = lua_touserdata(L, -2);
    HAPAssert(data);
    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 2);

    if (len == 0) {
        goto success;
    }

    luaL_addsize(B, len);

    size_t maxlen = B->size;
    len = luaL_bufflen(B);
    bool all = lua_toboolean(L, 3);
    if (len == maxlen || (!all && len != 0 && !pal_socket_readable(client->sock))) {
        goto success;
    }

    return lstream_client_async_read(L, client, maxlen - len, finshread);

success:
    luaL_pushresult(B);
    lua_pushstring(L, "");
    lua_setiuservalue(L, 1, 2);
    return 1;
}

static int lstream_client_async_read(lua_State *L, lstream_client *client, size_t maxlen, lua_KFunction k) {
    luaL_Buffer *B = &client->B;
    size_t len = maxlen;
    pal_err err = pal_socket_recv(client->sock, luaL_buffaddr(B) + luaL_bufflen(B),
        &len, lstream_client_read_recved_cb, client);
    if (err == PAL_ERR_IN_PROGRESS) {
        client->co = L;
        return lua_yieldk(L, 0, (lua_KContext)client, k);
    }

    int narg = 1;
    if (err == PAL_ERR_OK) {
        narg = 3;
        lua_pushlightuserdata(L, luaL_buffaddr(B) + luaL_bufflen(B));
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

    if (luai_unlikely(lua_getiuservalue(L, 1, 2) != LUA_TSTRING)) {
        luaL_argerror(L, 1, "readbuf must be a string");
    }

    size_t len;
    const char *readbuf = lua_tolstring(L, -1, &len);
    if (len == maxlen || (!all && len > 0 && len < maxlen && !pal_socket_readable(client->sock))) {
        lua_pushstring(L, "");
        lua_setiuservalue(L, 1, 2);
        return 1;
    } else if (len > maxlen) {
        lua_pushlstring(L, readbuf + maxlen, len - maxlen);
        lua_setiuservalue(L, 1, 2);
        lua_pushlstring(L, readbuf, maxlen);
        return 1;
    }

    lua_pop(L, 1);
    luaL_Buffer *B = &client->B;
    luaL_buffinitsize(L, B, maxlen);
    B->size = maxlen;
    luaL_addlstring(B, readbuf, len);
    return lstream_client_async_read(L, client, maxlen - len, finshread);
}

static int finshreadall(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    luaL_Buffer *B = &client->B;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        if (err == PAL_ERR_TIMEOUT) {
            goto success;
        }
        luaL_pushresult(B);
        lua_setiuservalue(L, 1, 2);
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    void *data = lua_touserdata(L, -2);
    HAPAssert(data);
    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 2);

    if (len == 0) {
        goto success;
    }

    luaL_addsize(B, len);
    luaL_prepbuffsize(B, LSTREAM_FRAME_LEN);
    return lstream_client_async_read(L, client, LSTREAM_FRAME_LEN, finshreadall);

success:
    luaL_pushresult(B);
    lua_pushstring(L, "");
    lua_setiuservalue(L, 1, 2);
    return 1;
}

static int lstream_client_readall(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);

    if (luai_unlikely(lua_getiuservalue(L, 1, 2) != LUA_TSTRING)) {
        luaL_argerror(L, 1, "readbuf must be a string");
    }

    size_t len;
    const char *readbuf = lua_tolstring(L, -1, &len);

    lua_pop(L, 1);
    luaL_Buffer *B = &client->B;
    luaL_buffinit(L, B);
    luaL_addlstring(B, readbuf, len);
    luaL_prepbuffsize(B, LSTREAM_FRAME_LEN);
    return lstream_client_async_read(L, client, LSTREAM_FRAME_LEN, finshreadall);
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

static bool lstream_client_getline(lua_State *L, bool skip, const char *data, size_t len,
    const char *sep, size_t seplen, size_t init) {
    const char *s = memfind(data + init, len - init, sep, seplen);
    if (s) {
        lua_pushlstring(L, s + seplen, len - (s + seplen - data));
        lua_setiuservalue(L, 1, 2);
        lua_pushlstring(L, data, skip ? s - data : s - data + seplen);
        return true;
    }
    return false;
}

static int finshreadline(lua_State *L, int status, lua_KContext extra) {
    lstream_client *client = (lstream_client *)extra;
    luaL_Buffer *B = &client->B;
    client->co = NULL;

    pal_err err = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (luai_unlikely(err != PAL_ERR_OK)) {
        luaL_pushresult(B);
        lua_setiuservalue(L, 1, 2);
        lua_pushstring(L, pal_err_string(err));
        return lua_error(L);
    }

    void *data = lua_touserdata(L, -2);
    HAPAssert(data);
    size_t len = lua_tointeger(L, -1);
    lua_pop(L, 2);

    if (len == 0) {
        luaL_pushresult(B);
        lua_setiuservalue(L, 1, 2);
        lua_pushstring(L, "read EOF");
        return lua_error(L);
    }

    size_t seplen;
    const char *sep = luaL_optlstring(L, 2, "\n", &seplen);
    bool skip = lua_toboolean(L, 3);
    size_t init = 0;
    if (seplen < luaL_bufflen(B)) {
        init = luaL_bufflen(B) + 1 - seplen;
    }

    luaL_addsize(B, len);

    if (lstream_client_getline(L, skip, luaL_buffaddr(B),
        luaL_bufflen(B), sep, seplen, init)) {
        return 1;
    }

    luaL_prepbuffsize(B, LSTREAM_LINE_LEN);
    return lstream_client_async_read(L, client, LSTREAM_LINE_LEN, finshreadline);
}

static int lstream_client_readline(lua_State *L) {
    lstream_client *client = lstream_client_get(L, 1);
    size_t seplen;
    const char *sep = luaL_optlstring(L, 2, "\n", &seplen);
    bool skip = lua_toboolean(L, 3);

    if (luai_unlikely(lua_getiuservalue(L, 1, 2) != LUA_TSTRING)) {
        luaL_argerror(L, 1, "readbuf must be a string");
    }

    size_t init = 0;
    size_t len;
    const char *readbuf = lua_tolstring(L, -1, &len);
    if (len > 0 && lstream_client_getline(L, skip, readbuf, len, sep, seplen, init)) {
        return 1;
    }

    lua_pop(L, 1);
    luaL_Buffer *B = &client->B;
    luaL_buffinit(L, B);
    luaL_addlstring(B, readbuf, len);
    luaL_prepbuffsize(B, LSTREAM_LINE_LEN);
    return lstream_client_async_read(L, client, LSTREAM_LINE_LEN, finshreadline);
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
