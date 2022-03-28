local httpc = require "httpc"
local urllib = require "url"
local hash = require "hash"
local cjson = require "cjson"
local base64 = require "base64"
local arc4 = require "arc4"
local bin2hex = require "util".bin2hex
local random = math.random
local tinsert = table.insert
local tunpack = table.unpack
local tconcat = table.concat
local floor = math.floor
local schar = string.char
local pairs = pairs
local error = error

local M = {}

---@class CloudSession:CloudSessionPriv Cloud API session.
local session = {}

local function buildCookie(tab)
    local strs = {}
    for k, v in pairs(tab) do
        tinsert(strs, ("%s=%s"):format(k, v))
    end
    return tconcat(strs, ";")
end

local function randomBytes(m, n, count)
    local bytes = {}
    for i = 1, count do
        tinsert(bytes, random(m, n))
    end
    return schar(tunpack(bytes))
end

local function genSignature(method, path, signed_nonce, query)
    local strs = {method, path}
    local keys = urllib.sortQueryKeys(query)
    for _, key in pairs(keys) do
        tinsert(strs, ("%s=%s"):format(key, query[key]))
    end
    tinsert(strs, base64.encode(signed_nonce))
    local s = tconcat(strs, "&")
    return base64.encode(hash.create("SHA1"):update(s):digest())
end

local function genNonce()
    return randomBytes(0, 255, 8) .. string.pack(">I4", floor(core.time() / 60000))
end

local function signNonce(ssecurity, nonce)
    return hash.create("SHA256"):update(ssecurity .. nonce):digest()
end

function session:loginStep1()
    local code, _, body = self.session:request(
        "GET", "https://account.xiaomi.com/pass/serviceLogin?sid=xiaomiio&_json=true", 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = buildCookie(self.cookie) .. ";userId=" .. self.username,
    })
    if code ~= 200 or body:find("_sign", 1, true) == nil then
        error("invalid username")
    end
    self.sign = body:match("\"_sign\":\"(.-)\"")
end

function session:loginStep2()
    local code, _, body = self.session:request(
        "POST", "https://account.xiaomi.com/pass/serviceLoginAuth2?" .. urllib.buildQuery({
            sid = "xiaomiio",
            hash = bin2hex(hash.create("MD5"):update(self.password):digest()):upper(),
            callback = "https://sts.api.io.mi.com/sts",
            qs = "%3Fsid%3Dxiaomiio%26_json%3Dtrue",
            user = self.username,
            _sign = self.sign,
            _json = true
        }), 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = buildCookie(self.cookie),
    })
    body = body:gsub("&&&START&&&", "")
    local resp = cjson.decode(body)
    if code ~= 200 or resp.ssecurity == nil or #resp.ssecurity < 4 then
        error("invalid login or password")
    end
    self.ssecurity = base64.decode(resp.ssecurity)
    self.userId = math.tointeger(resp.userId)
    self.cUserId = resp.cUserId
    self.passToken = resp.passToken
    self.location = resp.location
end

function session:loginStep3()
    local code, headers, body = self.session:request(
        "GET", self.location, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = buildCookie(self.cookie) .. ";userId=" .. self.username,
    })
    if code ~= 200 then
        error("unable to get service token")
    end
    self.serviceToken = headers["set-cookie"]:match("serviceToken=(.-);")
    base64.decode(self.serviceToken)
    self.location = nil
    self.sign = nil
end

---Login.
function session:login()
    self:loginStep1()
    self:loginStep2()
    self:loginStep3()
end

---Call cloud API.
---@param path string
---@param params table
---@return table result
function session:call(path, params)
    print(params.data)
    local cookie = {
        userId = self.userId,
        yetAnotherServiceToken = self.serviceToken,
        serviceToken = self.serviceToken,
        locale = "en_GB",
        timezone = "GMT+02:00",
        is_daylight = "1",
        dst_offset = "3600000",
        channel = "MI_APP_STORE"
    }
    local nonce = genNonce()
    local signedNonce = signNonce(self.ssecurity, nonce)
    params.rc4_hash__ = genSignature("POST", path, signedNonce, params)
    for k, v in pairs(params) do
        params[k] = base64.encode(arc4.create(signedNonce, 1024):crypt(v))
    end
    params.signature = genSignature("POST", path, signedNonce, params)
    params.ssecurity = base64.encode(self.ssecurity)
    params._nonce = base64.encode(nonce)
    local url = ("https://%sapi.io.mi.com/app%s?%s"):format(
        self.server == "cn" and "" or self.server .. ".",
        path, urllib.buildQuery(params))
    local code, headers, body = self.session:request(
        "POST", url, 5000, {
        ["Accept-Encoding"] = "identity",
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = buildCookie(self.cookie) .. ";" .. buildCookie(cookie),
        ["x-xiaomi-protocal-flag-cli"] = "PROTOCAL-HTTP2",
        ["MIOT-ENCRYPT-ALGORITHM"] = "ENCRYPT-RC4",
    })
    if code ~= 200 then
        error(cjson.decode(body).message)
    end
    body = arc4.create(signedNonce, 1024):crypt(base64.decode(body))
    local resp = cjson.decode(body)
    if resp.code ~= 0 then
        error(resp.message)
    end
    return resp.result
end

---Get miIO devices.
---@return table
function session:getDevices()
    return self:call("/home/device_list", {
        data = '{"getVirtualModel":true,"getHuamiDevices":1,"get_split_device":false,"support_smart_home":true}'
    })
end

---Get BlueTooth beacon key.
---@param did string Device ID.
function session:getBeaconKey(did)
    return self:call("/v2/device/blt_get_beaconkey", {
        data = '{"did":"' .. did .. '","pdid":1}'
    })
end

---Get device.
---@param did string Device ID.
function session:getDevice(did)
    return self:call("/v2/device", {
        data = '{"did":"' .. did .. '","pdid":1}'
    })
end

---New a session.
---@param server '"cn"'|'"de"'|'"us"'|'"ru"'|'"tw"'|'"sg"'|'"in"'|'"i2"'
---@param username string
---@param password string
---@return CloudSession session
function M.session(server, username, password)
    ---@class CloudSessionPriv
    local o = {
        server = server,
        username = username,
        password = password,
        agent = ("Android-7.1.1-1.0.0-ONEPLUS A3010-136-%s APP/xiaomi.smarthome APPV/62830"):format(randomBytes(65, 69, 13)),
        cookie = {
            sdkVersion = "accountsdk-18.8.15",
            deviceId = randomBytes(97, 122, 6)
        },
        session = httpc.session()
    }

    return setmetatable(o, {
        __index = session
    })
end

return M
