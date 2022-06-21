local httpc = require "httpc"
local urllib = require "url"
local hash = require "hash"
local cjson = require "cjson"
local base64 = require "base64"
local arc4 = require "arc4"
local nvs = require "nvs"
local random = math.random
local tinsert = table.insert
local tunpack = table.unpack
local tconcat = table.concat
local floor = math.floor
local schar = string.char
local pairs = pairs
local error = error

local M = {}

---@class MiCloudSession Xiaomi Cloud API session.
local session = {}

---Check the server region.
---@param region string
---@return boolean
local function checkRegion(region)
    if type(region) ~= "string" then
        return false
    end
    for _, v in ipairs({"cn", "de", "us", "ru", "tw", "sg", "in", "i2"}) do
        if region == v then
            return true
        end
    end
    return false
end

---Get miIO devices.
---@param type? '"wifi"'|'"zigbee"'|'"bluetooth"'
---@return integer pid
local function getPidByType(type)
    if type == "wifi" then
        return 0
    elseif type == "zigbee" then
        return 3
    elseif type == "bluetooth" then
        return 6
    end
end

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

---Login step 1.
---@return string sign
function session:loginStep1()
    local code, _, body = self.session:request(
        "GET", "https://account.xiaomi.com/pass/serviceLogin?sid=xiaomiio&_json=true", 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie .. ";userId=" .. self.username,
    })
    assert(body)
    if code ~= 200 or body:find("_sign", 1, true) == nil then
        error("invalid username")
    end
    return body:match("\"_sign\":\"(.-)\"")
end

---Login step 2.
---@param sign string
---@return string location
function session:loginStep2(sign)
    local code, _, body = self.session:request(
        "POST", "https://account.xiaomi.com/pass/serviceLoginAuth2?" .. urllib.buildQuery({
            sid = "xiaomiio",
            hash = self.password,
            callback = "https://sts.api.io.mi.com/sts",
            qs = "%3Fsid%3Dxiaomiio%26_json%3Dtrue",
            user = self.username,
            _sign = sign,
            _json = true
        }), 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
    })
    assert(body)
    local ssecurity = body:match("\"ssecurity\":\"(.-)\"")
    if code ~= 200 or ssecurity == nil or #ssecurity < 4 then
        error("invalid login or password")
    end
    self.ssecurity = ssecurity
    self.userId = body:match("\"userId\":(%d+)")
    return body:match("\"location\":\"(.-)\"")
end

---Loigin step 3.
---@param location string
function session:loginStep3(location)
    local code, headers = self.session:request(
        "GET", location, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
    })
    if code ~= 200 then
        error("unable to get service token")
    end
    for _, setcookie in ipairs(headers["Set-Cookie"]) do
        local serviceToken = setcookie:match("serviceToken=(.-);")
        if serviceToken then
            self.serviceToken = serviceToken
            return
        end
    end
    error("no service token in response")
end

---Login.
---@return MiCloudSession
function session:login()
    assert(self:isLogin() == false, "attempt to login repeatedly")
    self:loginStep3(self:loginStep2(self:loginStep1()))
    local handle <close> = nvs.open("miio.cloudapi")
    handle:set(self.username:sub(1, 15), {
        agentId = self.agentId,
        deviceId = self.deviceId,
        serviceToken = self.serviceToken,
        ssecurity = self.ssecurity,
        userId = self.userId
    })
    handle:commit()
    return self
end

function session:logout()
    if not self:isLogin() then
        return
    end
    self.serviceToken = nil
    self.ssecurity = nil
    self.userId = nil
    local handle <close> = nvs.open("miio.cloudapi")
    handle:set(self.username:sub(1, 15), nil)
    handle:commit()
end

---Is login.
---@return boolean
function session:isLogin()
    return self.serviceToken ~= nil
end

---Private request.
---@param path string
---@param data any
---@param encrypt? boolean
---@return table result
function session:_request(path, data, encrypt)
    assert(self:isLogin(), "attemp to request before login")
    local ssecurity = base64.decode(self.ssecurity)
    local nonce = genNonce()
    local signedNonce = signNonce(ssecurity, nonce)
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
        params.ssecurity = base64.encode(ssecurity)
    else
        params.signature = genSignature(path, nonce, signedNonce, params)
    end
    params._nonce = base64.encode(nonce)
    local url = ("https://%sapi.io.mi.com/app%s?%s"):format(
        self.region == "cn" and "" or self.region .. ".",
        path, urllib.buildQuery(params))
    local code, _, body = self.session:request(
        "POST", url, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie .. ";" .. buildCookie({
            userId = self.userId,
            yetAnotherServiceToken = self.serviceToken,
            serviceToken = self.serviceToken,
            locale = "zn_CN",
            timezone = "GMT+08:00",
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

---Request.
---@param path string
---@param data any
---@param encrypt? boolean
---@return table result
function session:request(path, data, encrypt)
    local retry = true

::again::
    local success, result = pcall(self._request, self, path, data, encrypt)
    if success == false then
        if retry then
            retry = false
            self:logout()
            self:login()
            goto again
        end
        error(result)
    end

    return result
end

---Get miIO devices.
---@param type? '"wifi"'|'"zigbee"'|'"bluetooth"'
---@return table
function session:getDevices(type)
    local pid = type and getPidByType(type) or nil
    return self:request("/home/device_list", pid and {
        pid = { pid },
    } or {}).list
end

---Get miIO devices by device ID.
---@param dids? string[]
---@return table
function session:getDevicesByID(dids)
    return self:request("/home/device_list", {
        dids = dids
    }).list
end

---Get BlueTooth beacon key.
---@param did string Device ID.
function session:getBeaconKey(did)
    return self:request("/v2/device/blt_get_beaconkey", {
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
    assert(checkRegion(region), "region must be one of ['cn', 'de', 'us', 'ru', 'tw', 'sg', 'in', 'i2']")
    assert(type(username) == "string", "username must be a string")
    assert(type(password) == "string", "password must be a string")

    local cache
    do
        local handle <close> = nvs.open("miio.cloudapi")
        cache = handle:get(username:sub(1, 15)) or {}
    end
    local agentId = cache.agentid or randomBytes(65, 69, 13)
    local deviceId = cache.agentid or randomBytes(97, 122, 6)

    ---@class MiCloudSession
    local o = {
        region = region,
        username = username,
        agentId = agentId,
        deviceId = deviceId,
        password = hash.create("MD5"):update(password):hexdigest():upper(),
        agent = ("Android-7.1.1-1.0.0-ONEPLUS A3010-136-%s APP/xiaomi.smarthome APPV/62830"):format(agentId),
        cookie = buildCookie({
            sdkVersion = "accountsdk-18.8.15",
            deviceId = deviceId,
        }),
        ssecurity = cache.ssecurity,
        userId = cache.userId,
        serviceToken = cache.serviceToken,
        session = httpc.session()
    }

    setmetatable(o, {
        __index = session,
        __close = session.close
    })

    return o:isLogin() and o or o:login()
end

return M
