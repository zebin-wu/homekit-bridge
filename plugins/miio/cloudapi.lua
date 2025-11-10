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

---Get PID by type.
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
    error("invalid type")
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

local function parseUTC(s)
    local dow, day, mon, year, hour, min, sec =
        s:match("(%a+), (%d+) (%a+) (%d+) (%d+):(%d+):(%d+) GMT")

    local months = {
        Jan=1, Feb=2, Mar=3, Apr=4, May=5, Jun=6,
        Jul=7, Aug=8, Sep=9, Oct=10, Nov=11, Dec=12
    }

    local utc = os.time({
        year  = tonumber(year),
        month = months[mon],
        day   = tonumber(day),
        hour  = tonumber(hour),
        min   = tonumber(min),
        sec   = tonumber(sec),
        isdst = false,
    })

    return utc
end

---Login step 1.
---@return string sign
function session:loginStep1()
    local code, headers, body = self.session:request(
        "GET", "https://account.xiaomi.com/pass/serviceLogin?sid=xiaomiio&_json=true", 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie .. ";userId=" .. self.username,
    })
    assert(body)
    if code ~= 200 or body:find("_sign", 1, true) == nil then
        error("invalid username")
    end
    self.tsOffset = parseUTC(headers["Date"]) * 1000 - core.time() / 1000
    return body:match("\"_sign\":\"(.-)\"")
end

local function readCache(username)
    local handle <close> = nvs.open("miio.cloudapi")
    return handle:get(username:sub(1, 15)) or {}
end

local function saveCache(username, cache)
    local handle <close> = nvs.open("miio.cloudapi")
    handle:set(username:sub(1, 15), cache)
    handle:commit()
end

function session:saveCache()
    saveCache(self.username:sub(1, 15), self.cache)
end

function session:resetCache()
    self.cache.serviceToken = nil
    self.cache.ssecurity = nil
    self.cache.userId = nil
    self.cache.verifyUrl = nil
    self.cache.captcode = nil
    self.cache.ick = nil
    self:saveCache()
end

---Get captcha image.
---@param captchaUrl string
---@return string captchaImg raw data of the captcha image
function session:getCaptcha(captchaUrl)
    local code, headers, body = self.session:request(
        "GET", captchaUrl, 5000)
    assert(headers["Set-Cookie"], "missing cookie")
    assert(body)

    local ick
    for _, setcookie in ipairs(headers["Set-Cookie"]) do
        ick = setcookie:match("ick=(.-);")
        if ick then
            self.cache.ick = ick
            self:saveCache()
            break
        end
    end
    assert(ick, "missing ick")
    return body
end

---Login step 2.
---@param sign string
---@return string location
function session:loginStep2(sign)
    local params = {
        sid = "xiaomiio",
        _dc = self.tsOffset + core.time(),
        hash = self.password,
        callback = "https://sts.api.io.mi.com/sts",
        qs = "%3Fsid%3Dxiaomiio%26_json%3Dtrue",
        user = self.username,
        _sign = sign,
        _json = true
    }
    local cookie = self.cookie
    if self.cache.captcode then
        params.captCode = self.cache.captcode
        cookie = cookie .. ";ick=" .. self.cache.ick
        self.cache.ick = nil
        self.cache.captcode = nil
    end
    local code, _, body = self.session:request(
        "POST", "https://account.xiaomi.com/pass/serviceLoginAuth2?" .. urllib.buildQuery(params), 10000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = cookie,
    })
    assert(body)
    if code ~= 200 then
        error("invalid login or password")
    end
    local ssecurity = body:match("\"ssecurity\":\"(.-)\"")
    if ssecurity == nil then
        local notificationUrl = body:match("\"notificationUrl\":\"(.-)\"")
        if notificationUrl then
            self.cache.verifyUrl = notificationUrl
            self:saveCache()
            error(("Two factor authentication required, please visit the following url and retry login: %s"):format(notificationUrl))
        end
        local captchaUrl = body:match("\"captchaUrl\":\"(.-)\"")
        if captchaUrl then
            if not captchaUrl:find("^http") then
                captchaUrl = "https://account.xiaomi.com" .. captchaUrl
            end
            local img = base64.encode(self:getCaptcha(captchaUrl))
            -- steps:
            -- 1. paste the following captcha code link into the input box in your browser and press Enter
            -- 2. use command "homekit-bridge config miio.captcode <captcode>" to set captcode on your device
            -- 3. use command "restart" to try again
            error(("Need captcha code, please visit the following captcha code: data:image/jpeg;base64,%s"):format(img))
        end
        error("invalid login or password")
    end
    self.cache.ssecurity = ssecurity
    self.cache.userId = body:match("\"userId\":(%d+)")
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
            self.cache.serviceToken = serviceToken
            return
        end
    end
    error("no service token in response")
end

---Check identity list from verify url
---@param url string
---@return string identitySession
---@return number[] options
function session:checkIdentityList(url)
    local code, headers, body = self.session:request(
        "GET", url:gsub("fe/service/identity/authStart", "identity/list"), 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
    })
    if code ~= 200 then
        error("unable to get identity list")
    end

    assert(body)

    if headers["Date"] then
        self.tsOffset = parseUTC(headers["Date"]) * 1000 - core.time() / 1000
    end

    assert(type(headers["Set-Cookie"]) == "string")
    local identitySession = headers["Set-Cookie"]:match("identity_session=(.-);")
    if identitySession == nil then
        error("missing identity_session")
    end

    local options = body:match("\"options\":(.-),")
    options = options and cjson.decode(options) or { tonumber(body:match("\"flag\":(%d+)")) }
    if options == nil or options == {} then
        error("missing options or flag")
    end

    return identitySession, options
end

---Verify ticket
---@param ticket string
---@return string location
function session:verifyTicket(ticket)
    if not self.cache.verifyUrl then
        return ""
    end

    local identitySession, options = self:checkIdentityList(self.cache.verifyUrl)
    for _, flag in ipairs(options) do
        local path
        if flag == 4 then
            path = "/identity/auth/verifyPhone"
        elseif flag == 8 then
            path = "/identity/auth/verifyEmail"
        end

        local code, headers, body = self.session:request(
            "POST", "https://account.xiaomi.com" .. path .. "?" .. urllib.buildQuery({
                _dc = self.tsOffset + core.time(),
                _flag = flag,
                ticket = ticket,
                trust = true,
                _json = true,
            }), 5000, {
            ["User-Agent"] = self.agent,
            ["Content-Type"] = "application/x-www-form-urlencoded",
            ["Cookie"] = self.cookie .. ";identity_session=" .. identitySession,
        })
        assert(body)
        if code == 200 and tonumber(body:match("\"code\":(%d+)")) == 0 then
            self.cache.verifyUrl = nil
            return body:match("\"location\":\"(.-)\"")
        end
    end

    error("verify failed")
end

function session:accountGet(url)
    local code, headers, body = self.session:request(
        "GET", url, 5000, {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie,
    })

    if code == 302 then
        self:accountGet(headers["Location"])
    elseif code ~= 200 then
        error("failed to get account")
    end
end

---Login.
---@return MiCloudSession
function session:login(ticket)
    assert(self:isLogin() == false, "attempt to login repeatedly")
    local location = nil
    if ticket then
        location = self:verifyTicket(ticket)
        self:accountGet(location)
    end
    self:loginStep3(self:loginStep2(self:loginStep1()))
    self:saveCache()
    return self
end

function session:logout()
    if not self:isLogin() then
        return
    end
    self:resetCache()
end

---Is login.
---@return boolean
function session:isLogin()
    return self.cache.serviceToken ~= nil
end

---Private request.
---@param path string
---@param data any
---@param encrypt? boolean
---@return table result
function session:_request(path, data, encrypt)
    assert(self:isLogin(), "attemp to request before login")
    local ssecurity = base64.decode(self.cache.ssecurity)
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
    local headers = {
        ["User-Agent"] = self.agent,
        ["Content-Type"] = "application/x-www-form-urlencoded",
        ["Cookie"] = self.cookie .. ";" .. buildCookie({
            userId = self.cache.userId,
            yetAnotherServiceToken = self.cache.serviceToken,
            serviceToken = self.cache.serviceToken,
            locale = "zn_CN",
            timezone = "GMT+08:00",
            is_daylight = "1",
            dst_offset = "3600000",
            channel = "MI_APP_STORE"
        }),
        ["x-xiaomi-protocal-flag-cli"] = "PROTOCAL-HTTP2"
    }
    if encrypt then
        headers["MIOT-ENCRYPT-ALGORITHM"] = "ENCRYPT-RC4"
    end
    local code, _, body = self.session:request("POST", url, 5000, headers)
    if code ~= 200 then
        error(cjson.decode(assert(body)).message)
    end
    assert(body, "missing body")
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
    if success == false and result ~= "timeout" then
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
---@param ticket string 2FA verify ticket
---@param captcode string Captcha code.
---@return MiCloudSession session
function M.session(region, username, password, ticket, captcode)
    assert(checkRegion(region), "region must be one of ['cn', 'de', 'us', 'ru', 'tw', 'sg', 'in', 'i2']")
    assert(type(username) == "string", "username must be a string")
    assert(type(password) == "string", "password must be a string")

    local cache = readCache(username:sub(1, 15))
    cache.captcode = captcode
    cache.agentId = cache.agentId or randomBytes(65, 69, 13)
    cache.deviceId = cache.deviceId or randomBytes(97, 122, 6)

    ---@class MiCloudSession
    local o = {
        tsOffset = 0,
        region = region,
        username = username,
        password = hash.create("MD5"):update(password):hexdigest():upper(),
        agent = ("Android-7.1.1-1.0.0-ONEPLUS A3010-136-%s APP/xiaomi.smarthome APPV/62830"):format(cache.agentId),
        cookie = buildCookie({
            sdkVersion = "3.8.6",
            deviceId = cache.deviceId,
        }),
        cache = cache,
        ticket = ticket,
        session = httpc.session()
    }

    setmetatable(o, {
        __index = session,
        __close = session.close
    })

    return o:isLogin() and o or o:login(ticket)
end

return M
