local httpc = require "httpc"
local urllib = require "url"
local hash = require "hash"
local cjson = require "cjson"
local base64 = require "base64"
local arc4 = require "arc4"
local random = math.random
local tointeger = math.tointeger
local tinsert = table.insert
local tunpack = table.unpack
local tconcat = table.concat
local floor = math.floor
local schar = string.char
local pairs = pairs
local error = error

local M = {}

---@class MiCloudSession:MiCloudSessionPriv Xiaomi Cloud API session.
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

local function genSignatureARC4(method, path, signed_nonce, query)
    local strs = {method, path}
    local keys = urllib.sortQueryKeys(query)
    for _, key in pairs(keys) do
        tinsert(strs, ("%s=%s"):format(key, query[key]))
    end
    tinsert(strs, base64.encode(signed_nonce))
    local s = tconcat(strs, "&")
    return base64.encode(hash.create("SHA1"):update(s):digest())
end

local function genSignature(path, nonce, signNonce, query)
    local strs = {path, base64.encode(signNonce), base64.encode(nonce)}
    local keys = urllib.sortQueryKeys(query)
    for _, key in pairs(keys) do
        tinsert(strs, ("%s=%s"):format(key, query[key]))
    end
    local s = tconcat(strs, "&")
    return base64.encode(hash.create("SHA256", signNonce):update(s):digest())
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
        ["Cookie"] = self.cookie .. ";userId=" .. self.username,
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
            hash = self.password,
            callback = "https://sts.api.io.mi.com/sts",
            qs = "%3Fsid%3Dxiaomiio%26_json%3Dtrue",
            user = self.username,
            _sign = self.sign,
            _json = true
        }), 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
    })
    body = body:gsub("&&&START&&&", "")
    local resp = cjson.decode(body)
    if code ~= 200 or resp.ssecurity == nil or #resp.ssecurity < 4 then
        error("invalid login or password")
    end
    self.ssecurity = base64.decode(resp.ssecurity)
    self.userId = tointeger(resp.userId)
    self.cUserId = resp.cUserId
    self.passToken = resp.passToken
    self.location = resp.location
end

function session:loginStep3()
    local code, headers, body = self.session:request(
        "GET", self.location, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
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
---@return MiCloudSession
function session:login()
    self:loginStep1()
    self:loginStep2()
    self:loginStep3()
    return self
end

---Request.
---@param path string
---@param data any
---@param encrypt? boolean
---@return table result
function session:request(path, data, encrypt)
    if self.serviceToken == nil then
        error("attemp to call api before login")
    end
    local nonce = genNonce()
    local signedNonce = signNonce(self.ssecurity, nonce)
    local rc4ctx
    local params = {
        data = cjson.encode(data)
    }
    if encrypt then
        params.rc4_hash__ = genSignatureARC4("POST", path, signedNonce, params)
        rc4ctx = arc4.create(signedNonce, 1024)
        for k, v in pairs(params) do
            params[k] = base64.encode(rc4ctx:crypt(v))
            rc4ctx:reset()
        end
        params.signature = genSignatureARC4("POST", path, signedNonce, params)
        params.ssecurity = base64.encode(self.ssecurity)
    else
        params.signature = genSignature(path, nonce, signedNonce, params)
    end
    params._nonce = base64.encode(nonce)
    local url = ("https://%sapi.io.mi.com/app%s?%s"):format(
        self.region == "cn" and "" or self.region .. ".",
        path, urllib.buildQuery(params))
    local code, headers, body = self.session:request(
        "POST", url, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie .. ";" .. buildCookie({
            userId = self.userId,
            yetAnotherServiceToken = self.serviceToken,
            serviceToken = self.serviceToken,
            locale = "en_GB",
            timezone = "GMT+02:00",
            is_daylight = "1",
            dst_offset = "3600000",
            channel = "MI_APP_STORE"
        }),
        ["x-xiaomi-protocal-flag-cli"] = "PROTOCAL-HTTP2",
        ["MIOT-ENCRYPT-ALGORITHM"] = encrypt and "ENCRYPT-RC4" or nil,
    })
    if code ~= 200 then
        error(cjson.decode(body).message)
    end
    if encrypt then
        body = rc4ctx:crypt(base64.decode(body))
    end
    local resp = cjson.decode(body)
    if resp.code ~= 0 then
        error(resp.message)
    end
    return resp.result
end

---Get miIO devices.
---@param dids? string[]
---@return table
function session:getDevices(dids)
    return self:request("/home/device_list", dids and {
        dids = dids
    } or {
        getVirtualModel = false,
        getHuamiDevices = 1
    }).list
end

---Get BlueTooth beacon key.
---@param did string Device ID.
function session:getBeaconKey(did)
    return self:request("/v2/device/blt_get_beaconkey",     {
        did = did,
        pdid = 1,
    }).beaconkey
end

function session:close()
    self.session:close()
end

---New a session.
---@param region '"cn"'|'"de"'|'"us"'|'"ru"'|'"tw"'|'"sg"'|'"in"'|'"i2"' Server region.
---@param username string User ID or email.
---@param password string User password.
---@return MiCloudSession session
function M.session(region, username, password)
    ---@class MiCloudSessionPriv
    local o = {
        region = region,
        username = username,
        password = hash.create("MD5"):update(password):hexdigest():upper(),
        agent = ("Android-7.1.1-1.0.0-ONEPLUS A3010-136-%s APP/xiaomi.smarthome APPV/62830"):format(randomBytes(65, 69, 13)),
        cookie = buildCookie({
            sdkVersion = "accountsdk-18.8.15",
            deviceId = randomBytes(97, 122, 6)
        }),
        session = httpc.session()
    }

    return setmetatable(o, {
        __index = session,
        __close = session.close
    })
end

return M
