local tonumber = tonumber
local tostring = tostring
local type = type
local ipairs = ipairs
local pairs = pairs
local schar = string.char
local sbyte = string.byte
local tsort = table.sort
local tinsert = table.insert
local tconcat = table.concat

---URL library.
---
---Every HTTP URL conforms to the syntax of a generic URI.
---The URI generic syntax consists of a hierarchical sequence of five components:
---```
---URI = scheme ":" ["//" authority] path ["?" query] ["#" fragment]
---```
---where the authority component divides into three subcomponents:
---```
---authority = [userinfo "@"] host [":" port]
---```
---@class urllib
local M = {}

M.options = {
    query = {
        separator = "&",
        cumulativeParams = false,
        plusIsSpace = false,
        legal = {
            [":"] = true, ["-"] = true, ["_"] = true, ["."] = true,
            [","] = true, ["!"] = true, ["~"] = true, ["*"] = true,
            ["'"] = true, [";"] = true, ["("] = true, [")"] = true,
            ["@"] = true, ["$"] = true,
        }
    },
    path = {
        legal = {
            [":"] = true, ["-"] = true, ["_"] = true, ["."] = true,
            ["!"] = true, ["~"] = true, ["*"] = true, ["'"] = true,
            ["("] = true, [")"] = true, ["@"] = true, ["&"] = true,
            ["="] = true, ["$"] = true, [","] = true,
            [";"] = true
        }
    }
}

local function decode(s)
    return s:gsub("%%(%x%x)", function (c) return schar(tonumber(c, 16)) end)
end

local function encode(str, legal)
    return str:gsub("([^%w])", function (v)
        if legal[v] then
            return v
        end
        return ("%%%02x"):format(sbyte(v)):upper()
    end)
end

local function concat(a, b)
    if type(a) == "table" then
        return a:build() .. b
    else
        return a .. b:build()
    end
end

---@class URL:table URL object.
---
---@field scheme string
---@field userinfo string
---@field user string
---@field password string
---@field host string
---@field port string
---@field fragment string
local url = {}

---Add a segment to the path of the URL.
---@param path string
---@return URL
function url:addSegment(path)
    assert(type(path) == "string", "path must be a string")
    self.path = ("%s/%s"):format(self.path or "", encode(path:gsub("^/+", ""), M.options.path.legal))
    return self
end

---Set the URL query.
---@param query string|table
---@return table
function url:setQuery(query)
    if type(query) == "table" then
        query = M.buildQuery(query)
    end
    self.query = M.parseQuery(query)

    return query
end

---Set the authority part of the URL.
---@param authority string
function url:setAuthority(authority)
    self.authority = authority
    self.userinfo = nil
    self.user = nil
    self.password = nil
    self.host = nil
    self.port = nil

    authority = authority:gsub("^([^@]*)@", function (v)
        self.userinfo = v
        return ""
    end)

    authority = authority:gsub(":(%d+)$", function (v)
        self.port = tonumber(v)
        return ""
    end)

    if authority ~= "" then
        self.host = authority
    end

    if self.userinfo then
        local userinfo = self.userinfo
        userinfo = userinfo:gsub(":([^:]*)$", function (v)
            self.password = v
            return ''
        end)
        if string.find(userinfo, "^[%w%+%.]+$") then
            self.user = userinfo
        else
            -- incorrect userinfo
            self.userinfo = nil
            self.user = nil
            self.password = nil
        end
    end
end

---Build URL string.
---@return string
function url:build()
    local s = self.path or ""

    if self.query then
        local qstring = tostring(self.query)
        if qstring ~= "" then
            s = s .. "?" .. qstring
        end
    end

    if self.host then
        local authority = "//" .. self.host
        local port = self.port
        if port then
            authority = authority .. ":" .. port
        end
        local userinfo
        local user = self.user
        if user and user ~= "" then
            userinfo = user
            local password = self.password
            if password then
                userinfo = userinfo .. ":" .. password
            end
        end
        if userinfo and userinfo ~= "" then
            authority = userinfo .. "@" .. authority
        end
        if authority then
            if s ~= "" then
                s = authority .. "/" .. s:gsub("^/+", "")
            else
                s = authority
            end
        end
    end

    local scheme = self.scheme
    if scheme then s = scheme .. ":" .. s end
    local fragment = self.fragment
    if fragment then s = s .. "#" .. fragment end

    return s
end

---Build the query string.
---@param tab table
---@param sep? string
---@param key? string
---@return string
function M.buildQuery(tab, sep, key)
    local opt = M.options.query
    sep = sep or opt.separator or "&"

    local keys = {}
    for k in pairs(tab) do
        tinsert(keys, k)
    end
    tsort(keys, function (a, b)
          local function padnum(n, rest) return ("%03d"..rest):format(tonumber(n)) end
          return tostring(a):gsub("(%d+)(%.)", padnum) < tostring(b):gsub("(%d+)(%.)", padnum)
    end)

    local query = {}
    local legal = M.options.query.legal
    for _, name in ipairs(keys) do
        local value = tab[name]
        name = encode(tostring(name), {["-"] = true, ["_"] = true, ["."] = true})
        if key then
            if opt.cumulativeParams and name:find("^%d+$") then
                name = tostring(key)
            else
                name = ("%s[%s]"):format(tostring(key), tostring(name))
            end
        end
        if type(value) == "table" then
            tinsert(query, M.buildQuery(value, sep, name))
        else
            value = encode(tostring(value), legal)
            tinsert(query, value ~= "" and ("%s=%s"):format(name, value) or name)
        end
    end

    return tconcat(query, sep)
end

---Parses the query string to a table.
---@param s string
---@param sep string
---@return table
function M.parseQuery(s, sep)
    local opt = M.options.query
    sep = sep or opt.separator or "&"

    local _decode = opt.plusIsSpace and function (s)
        return decode(s:gsub("+", " "))
    end or decode

    local res = {}
    for key, val in s:gmatch(("([^%q=]+)(=*[^%q=]*)"):format(sep, sep)) do
        key = _decode(key)
        local keys = {}
        key = key:gsub("%[([^%]]*)%]", function (v)
            -- extract keys between balanced brackets
            if v:find("^-?%d+$") then
                v = tonumber(v)
            else
                v = _decode(v)
            end
            table.insert(keys, v)
            return "="
        end)
        key = key:gsub("=+.*$", ""):gsub("%s", "_")
        val = val:gsub("^=+", "")

        if not res[key] then
            res[key] = {}
        end
        if #keys > 0 and type(res[key]) ~= "table" then
            res[key] = {}
        elseif #keys == 0 and type(res[key]) == "table" then
            res[key] = _decode(val)
        elseif opt.cumulativeParams and type(res[key]) == "string" then
            res[key] = { res[key] }
            tinsert(res[key], _decode(val))
        end

        local t = res[key]
        for i, k in ipairs(keys) do
            if type(t) ~= 'table' then
                t = {}
            end
            if k == "" then
                k = #t + 1
            end
            if not t[k] then
                t[k] = {}
            end
            if i == #keys then
                t[k] = val
            end
            t = t[k]
        end
    end

    return setmetatable(res, { __tostring = M.buildQuery })
end

---Parse the URL into the designated parts.
---@param s string
---@return URL url
function M.parse(s)
    local res = {}

    s = s:gsub("#(.*)$", function (v)
        res.fragment = v
        return ""
    end)
    s = s:gsub("^([%w][%w%+%-%.]*)%:", function (v)
        res.scheme = v:lower()
        return ""
    end)
    s = s:gsub("%?(.*)", function (v)
        url.setQuery(res, v)
        return ""
    end)

    s = s:gsub("^//([^/]*)", function (v)
        url.setAuthority(res, v)
        return ""
    end)

    local legal = M.options.path.legal
    local path = s:gsub("([^/]+)", function (v)
        return encode(decode(v), legal)
    end)
    if path ~= "" then
        res.path = path
    end

    return setmetatable(res, {
        __index = url,
        __tostring = url.build,
        __concat = concat,
    })
end

return M
