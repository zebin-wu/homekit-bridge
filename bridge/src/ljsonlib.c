// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>
#include <HAPBase.h>

#define LJSON_TOKEN_DEPTH 128
#define LJSON_OBJ_NAME "JSON*"
#define LJSON_STR_LEN(start, end) ((end) - (start) + 1)

HAP_ENUM_BEGIN(uint8_t, ljson_type) {
    LJSON_TYPE_OBJECT,
    LJSON_TYPE_ARRAY,
} HAP_ENUM_END(uint8_t, ljson_type);

typedef struct {
    ljson_type type;
    size_t start;
    size_t end;
    size_t pos;
} ljson;

static inline bool ljson_iswhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == 'r';
}

static inline void ljson_push_true(lua_State *L) {
    lua_pushboolean(L, true);
}

static inline void ljson_push_false(lua_State *L) {
    lua_pushboolean(L, false);
}

static inline void ljson_push_null(lua_State *L) {
    lua_pushlightuserdata(L, NULL);
}

static int ljson_throw_parse_error(lua_State *L, const char *exp, const char *found, size_t pos) {
    return luaL_error(L, "expected %s but found %s at character %d", exp, found, pos);
}

static void ljson_parse_string(lua_State *L, const char *s, size_t start, size_t end) {
    HAPPrecondition(start != end && s[start] == '"' && s[end] == '"');

    start++;
    end--;

    luaL_Buffer B;
    luaL_buffinit(L, &B);

    for (; start <= end; start++) {
        char c = s[start];
        switch (c) {
        case '\\':
            start++;
            HAPAssert(start != end);
            c = s[start];
            switch (c) {
            case '"':
            case '\\':
            case '/':
                break;
            case 'b':
                c = '\b';
                break;
            case 't':
                c = '\t';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case 'f':
                c = '\f';
                break;
            case 'u':
            default:
                ljson_throw_parse_error(L, "escape code", "invalid escape code", start);
                break;
            }
            break;
        case '"':
            ljson_throw_parse_error(L, "value end", "'\"'", start + 1);
            break;
        default:
            break;
        }
        luaL_addchar(&B, c);
    }

    luaL_pushresult(&B);
}

static void ljson_parse_number(lua_State *L, const char *s, size_t start, size_t end) {
    char *endptr;

    double number = strtod(s + start, &endptr);
    if (s + start == endptr) {
        ljson_throw_parse_error(L, "number", "invalid number", start);
    }
    lua_pushnumber(L, number);
}

static ljson *ljson_create(lua_State *L, int index,
    ljson_type type, const char *s, size_t start, size_t end) {
    ljson *json = lua_newuserdatauv(L, sizeof(*json), 2);
    luaL_setmetatable(L, LJSON_OBJ_NAME);

    lua_pushvalue(L, index < 0 ? index - 1 : index);
    lua_setiuservalue(L, -2, 1);

    json->type = type;
    json->start = start;
    json->end = end;
    json->pos = start + 1;

    return json;
}

static void ljson_parse_value(lua_State *L, int index, const char *s, size_t start, size_t end) {
    if (LJSON_STR_LEN(start, end) >= 2 && (!s[0] || !s[1])) {
        luaL_error(L, "JSON parser does not support UTF-16 or UTF-32");
    }

    // skip space
    while (ljson_iswhitespace(s[start])) start++;
    while (ljson_iswhitespace(s[end])) end--;

    switch (s[start]) {
    case '{':  // object
        if (s[end] != '}') {
            ljson_throw_parse_error(L, "'}'", "invalid token", end);
        }
        ljson_create(L, index, LJSON_TYPE_OBJECT, s, start, end);
        break;
    case '[':  // array
        if (s[end] != ']') {
            ljson_throw_parse_error(L, "']'", "invalid token", end);
        }
        ljson_create(L, index, LJSON_TYPE_ARRAY, s, start, end);
        break;
    case '"':  // string
        if (s[end] != '"') {
            ljson_throw_parse_error(L, "'\"'", "invalid token", end);
        }
        ljson_parse_string(L, s, start, end);
        break;
    case 'f':  // false?
        if (strncmp("false", s + start, LJSON_STR_LEN(start, end))) {
            ljson_throw_parse_error(L, "value", "invalid token", start);
        }
        ljson_push_false(L);
        break;
    case 't':  // true?
        if (strncmp("true", s + start, LJSON_STR_LEN(start, end))) {
            ljson_throw_parse_error(L, "value", "invalid token", start);
        }
        ljson_push_true(L);
        break;
    case 'n': case 'N':  // null?
        if (strncmp("null", s + start, LJSON_STR_LEN(start, end))) {
            ljson_throw_parse_error(L, "value", "invalid token", start);
        }
        ljson_push_null(L);
        break;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':  // number?
        ljson_parse_number(L, s, start, end);
        break;
    default:
        ljson_throw_parse_error(L, "value", "invalid token", start);
        break;
    }
}

static int ljson_decode(lua_State *L) {
    luaL_argcheck(L, lua_gettop(L) == 1, 1, "expected 1 argument");

    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);

    ljson_parse_value(L, 1, s, 0, len - 1);
    return 1;
}

static size_t ljson_find_string_end(lua_State *L, const char *s, size_t pos, size_t end) {
    bool escape = false;

    pos++;

    for (; pos < end; pos++) {
        switch (s[pos]) {
        case '\\':
            escape = true;
            break;
        case '"':
            if (!escape) {
                return pos;
            }
            escape = false;
            break;
        default:
            escape = false;
            break;
        }
    }

    return ljson_throw_parse_error(L, "'\"'", "invalid token", pos);
}

static size_t ljson_find_value_end(lua_State *L, const char *s, size_t pos, size_t end) {
    char stack[LJSON_TOKEN_DEPTH];
    uint8_t top = 0;

    for (; pos < end; pos++) {
        switch (s[pos]) {
        case '"':
            pos = ljson_find_string_end(L, s, pos, end);
            break;
        case ',':
            if (top == 0) {
                return pos - 1;
            }
            break;
        case '{':
        case '[':
            if (top == LJSON_TOKEN_DEPTH - 1) {
                luaL_error(L, "JSON depth too large");
            }
            stack[top++] = s[pos] + 2;  // ']' or '}'
            break;
        case '}':
        case ']':
            if (top == 0) {
                return pos - 1;
            }
            if (stack[--top] != s[pos]) {
                char exp[2];
                exp[0] = stack[top];
                exp[1] = '\0';
                ljson_throw_parse_error(L, exp, "invalid token", pos);
            }
            break;
        default:
            break;
        }
    }

    return pos - 1;
}

static size_t ljson_parse_object(lua_State *L, int index, const char *s, size_t pos, size_t end) {
    size_t next;

    // find key
    if (s[pos] != '\"') {
        ljson_throw_parse_error(L, "'\"'", "invalid token", pos);
    }
    next = ljson_find_string_end(L, s, pos, end);
    ljson_parse_string(L, s, pos, next);
    pos = next + 1;

    // find :
    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end || s[pos] != ':') {
        ljson_throw_parse_error(L, "':'", "invalid token", pos);
    }
    pos++;

    // find value
    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end) {
        ljson_throw_parse_error(L, "value", "invalid token", pos);
    }
    next = ljson_find_value_end(L, s, pos, end);
    ljson_parse_value(L, index < 0 ? index - 1 : index, s, pos, next);
    pos = next + 1;

    if (pos == end) {
        return pos;
    }

    // find ,
    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end) {
        return pos;
    }
    if (s[pos] != ',') {
        ljson_throw_parse_error(L, "','", "invalid token", pos);
    }
    pos++;

    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    return pos;
}

static size_t ljson_parse_array(lua_State *L, int index, const char *s, size_t pos, size_t end) {
    size_t next;

    // find value
    next = ljson_find_value_end(L, s, pos, end);
    ljson_parse_value(L, index, s, pos, next);
    pos = next + 1;

    if (pos == end) {
        return pos;
    }

    // find ,
    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end) {
        return pos;
    }
    if (s[pos] != ',') {
        ljson_throw_parse_error(L, "','", "invalid token", pos);
    }
    pos++;

    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    return pos;
}

static void ljson_pushtable(lua_State *L, int index) {
    index = index < 0 ? lua_gettop(L) + 1 + index : index;

    if (lua_getiuservalue(L, index, 2) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setiuservalue(L, index, 2);
    }
}

static int ljson_index(lua_State *L) {
    ljson *json = luaL_checkudata(L, 1, LJSON_OBJ_NAME);
    switch (json->type) {
    case LJSON_TYPE_OBJECT:
        luaL_checkstring(L, 2);
        break;
    case LJSON_TYPE_ARRAY:
        luaL_checkinteger(L, 2);
        break;
    }

    ljson_pushtable(L, 1);

    lua_pushvalue(L, 2);
    if (lua_gettable(L, 3) != LUA_TNIL || json->pos == json->end) {
        return 1;
    }

    lua_getiuservalue(L, 1, 1);
    const char *s = lua_tostring(L, -1);

    size_t pos = json->pos;
    size_t end = json->end;

    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end) {
        goto end;
    }

    switch (json->type) {
    case LJSON_TYPE_OBJECT:
        for (bool loop = true; loop && pos < end;) {
            pos = ljson_parse_object(L, -1, s, pos, end);
            if (lua_rawequal(L, -2, 2)) {
                loop = false;
            }
            lua_settable(L, 3);
        }
        break;
    case LJSON_TYPE_ARRAY:
        lua_Integer max = lua_tointeger(L, 2);
        for (size_t i = lua_rawlen(L, 3) + 1; i <= max && pos < end; i++) {
            pos = ljson_parse_array(L, -1, s, pos, end);
            lua_seti(L, 3, i);
        }
        break;
    }

end:
    json->pos = pos;
    lua_pushvalue(L, 2);
    lua_gettable(L, 3);
    return 1;
}

static int ljson_next(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);
    if (lua_next(L, 1)) {
        return 2;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int ljson_pairs(lua_State *L) {
    ljson *json = luaL_checkudata(L, 1, LJSON_OBJ_NAME);
    size_t pos = json->pos;
    size_t end = json->end;

    lua_pushcfunction(L, ljson_next);

    ljson_pushtable(L, 1);
    if (pos == end) {
        goto end;
    }

    lua_getiuservalue(L, 1, 1);
    const char *s = lua_tostring(L, -1);

    while (pos < end && ljson_iswhitespace(s[pos])) pos++;
    if (pos == end) {
        lua_pop(L, 1);
        goto end;
    }

    switch (json->type) {
    case LJSON_TYPE_OBJECT:
        while (pos < end) {
            pos = ljson_parse_object(L, -1, s, pos, end);
            lua_settable(L, 3);
        }
        break;
    case LJSON_TYPE_ARRAY:
        for (size_t i = lua_rawlen(L, 3) + 1; pos < end; i++) {
            pos = ljson_parse_array(L, -1, s, pos, end);
            lua_seti(L, 3, i);
        }
        break;
    }

    lua_pop(L, 1);

end:
    json->pos = pos;
    lua_pushnil(L);
    return 3;
}

static int ljson_tostring(lua_State *L) {
    ljson *json = luaL_checkudata(L, 1, LJSON_OBJ_NAME);

    int t = lua_getiuservalue(L, 1, 1);
    HAPAssert(t == LUA_TSTRING);
    const char *s = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_pushlstring(L, s + json->start, LJSON_STR_LEN(json->start, json->end));
    return 1;
}

static const luaL_Reg ljson_funcs[] = {
    {"null", NULL},  /* place holder */
    {"decode", ljson_decode},
    {NULL, NULL},
};

/*
 * metamethods for JSON object
 */
static const luaL_Reg metameth[] = {
    {"__index", ljson_index},
    {"__pairs", ljson_pairs},
    {"__tostring", ljson_tostring},
    {NULL, NULL}
};

LUAMOD_API int luaopen_json(lua_State *L) {
    luaL_newlib(L, ljson_funcs);

    ljson_push_null(L);
    lua_setfield(L, -2, "null");

    luaL_newmetatable(L, LJSON_OBJ_NAME);  /* metatable for JSON object */
    luaL_setfuncs(L, metameth, 0);  /* add metamethods to new metatable */
    lua_pop(L, 1);  /* pop metatable */

    return 1;
}
