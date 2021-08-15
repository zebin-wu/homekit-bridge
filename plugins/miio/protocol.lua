local udp = require "net.udp"
local timer = require "timer"
local hash = require "hash"
local json = require "cjson"

local protocol = {}
local logger = log.getLogger("miio.protocol")

---
--- Message format
---
--- 0                   1                   2                   3
--- 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
--- +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--- | Magic number = 0x2131         | Packet Length (incl. header)  |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Unknown                                                       |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Device ID ("did")                                             |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Stamp                                                         |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | MD5 checksum                                                  |
--- | ... or Device Token in response to the "Hello" packet         |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | optional variable-sized data (encrypted)                      |
--- |...............................................................|
---
---                 Mi Home Binary Protocol header
---        Note that one tick mark represents one bit position.
---
---  Magic number: 16 bits
---      Always 0x2131
---
---  Packet length: 16 bits unsigned int
---      Length in bytes of the whole packet, including the header.
---
---  Unknown: 32 bits
---      This value is always 0,
---      except in the "Hello" packet, when it's 0xFFFFFFFF
---
---  Device ID: 32 bits
---      Unique number. Possibly derived from the MAC address.
---      except in the "Hello" packet, when it's 0xFFFFFFFF
---
---  Stamp: 32 bit unsigned int
---      continously increasing counter
---
---  MD5 checksum:
---      calculated for the whole packet including the MD5 field itself,
---      which must be initialized with 0.
---
---      In the special case of the response to the "Hello" packet,
---      this field contains the 128-bit device token instead.
---
---  optional variable-sized data:
---      encrypted with AES-128: see below.
---      length = packet_length - 0x20
---
---@class MiioMessage
---
---@field unknown integer
---@field did integer
---@field stamp integer
---@field checksum string
---@field data string

---
--- Encryption
---
--- The variable-sized data payload is encrypted with the Advanced Encryption Standard (AES).
---
--- A 128-bit key and Initialization Vector are both derived from the Token as follows:
---   Key = MD5(Token)
---   IV  = MD5(MD5(Key) + Token)
--- PKCS#7 padding is used prior to encryption.
---
--- The mode of operation is Cipher Block Chaining (CBC).
---
---@class MiioEncryption
---
---@field encrypt fun(input: string): string
---@field decrypt fun(input: string): string

---New a encryption.
---@param token string Device token.
---@return MiioEncryption encryption A new encryption.
local function newEncryption(token)
    local function md5(data)
        local m = hash.md5()
        m:update(token)
        return m:digest()
    end

    local cipher = require("cipher").create("AES-128-CBC")
    if not cipher then
        logger:error("Failed to create a AES-128-CBC cipher.")
        return nil
    end

    local key = md5(token)
    local iv = md5(key .. token)

    local o = {
        cipher = cipher,
        key = key,
        iv = iv,
    }

    o.cipher:setPadding("PKCS7")

    function o:encrypt(input)
        self.cipher:begin("encrypt", self.key, self.iv)
        return self.cipher:update(input) .. self.cipher:finsh()
    end

    function o:decrypt(input)
        self.cipher:begin("decrypt", self.key, self.iv)
        return self.cipher:update(input) .. self.cipher:finsh()
    end

    return o
end

---Pack a message to a binary package.
---@param unknown integer Unknown: 32-bit.
---@param did integer Device ID: 32-bit.
---@param stamp integer Stamp: 32 bit unsigned int.
---@param checksum string|nil MD5 checksnum: 128-bit.
---@param data? string Optional variable-sized data.
---@return string package
local function pack(unknown, did, stamp, checksum, data)
    local len = 32
    if data then
        len = len + #data
    end

    local header = string.pack(">I2>I2>I4>I4>I4", 0x2131, len, unknown, did, stamp)
    if checksum == nil then
        local md5 = hash.md5()
        md5:update(header)
        md5:update(string.pack(">I4>I4>I4>I4", 0, 0, 0, 0))
        if data then
            md5:update(data)
        end
        checksum = md5:digest()
    end
    assert(#checksum == 16)
    local fmt = "c" .. #header .. "c" .. #checksum
    if data then
        fmt = fmt .. "c" .. #data
    end

    return string.pack(fmt, header, checksum, data)
end

---Unpack a message from a binary package.
---@param package string A binary package.
---@return MiioMessage|nil message
local function unpack(package)
    if string.unpack(">I2", package, 1) ~= 0x2131 then
        return nil
    end

    local len = string.unpack(">I2", package, 3)
    if len < 32 or len ~= #package then
        return nil
    end

    local data = nil
    if len > 32 then
        data = string.unpack("c" .. len - 32, package, 33)
    end

    return {
        unknown = string.unpack(">I4", package, 5),
        did = string.unpack(">I4", package, 9),
        stamp = string.unpack(">I4", package, 13),
        checksum = string.unpack("c16", package, 17),
        data = data
    }
end

---Pack a hello package.
---@return string package
local function packHello()
    return pack(
        0xffffffff,
        0xffffffff,
        0xffffffff,
        string.pack(">I4>I4>I4>I4", 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff)
    )
end

---Scan for devices in the network.
---@param cb fun(addr: string, port: integer, devid: integer, stamp: integer) Function call when the device is scaned.
---@param timeout integer Timeout period (in seconds).
---@param addr? string Target Address.
---@return boolean status true on success, false on failure.
function protocol.scan(cb, timeout, addr)
    assert(type(cb) == "function")
    assert(timeout > 0, "the parameter 'timeout' must be greater then 0")

    local context = {}
    local handle = udp.open("inet")
    if not handle then
        logger:error("Failed to open a UDP handle.")
        return false
    end
    if not addr then
        if handle:enableBroadcast() == false then
            logger:error("Failed to enable UDP broadcast.")
            handle:close()
            return false
        end
    end
    local t = timer.create(timeout * 1000, function(context)
        logger:debug("Scan done.")
        context.handle:close()
    end, context)
    if not t then
        logger:error("Failed to create a timer.")
        handle:close()
        return false
    end
    if not handle:sendto(packHello(), addr or "255.255.255.255", 54321) then
        logger:error("Failed to send hello message.")
        handle:close()
        t:cancel()
        return false
    end
    handle:setRecvCb(function (data, from_addr, from_port, cb)
        local m = unpack(data)
        if m and m.unknown == 0 and m.data == nil then
            cb(from_addr, from_port, m.did, m.stamp)
        end
    end, cb)
    context.handle = handle
    context.timer = t
    return true
end

---comment
---@param addr string Device address.
---@param port integer Device port.
---@param devid integer Device ID.
---@param token string Device token.
---@param stamp integer Device time stamp obtained by scanning.
---@return table
function protocol.new(addr, port, devid, token, stamp, ...)
    assert(type(token) == "string" and #token == 16, "invalid token")

    local o = {
        stampDiff = os.time() - stamp,
        devid = devid,
        reqid = 0,
        respCb = nil,
        errCb = nil,
        args = { ... }
    }

    o.handle = udp.open("inet")
    if not o.handle then
        logger:error("Failed to open a UDP handle.")
        return nil
    end
    if not o.handle:connect(addr, port) then
        logger:error(("Failed to connect to %s:%d"):format(addr, port))
        o.handle:close()
        return nil
    end

    o.encryption = newEncryption(token)
    if not o.encryption then
        logger:error("Failed to new a encryption.")
        o.handle:close()
        return nil
    end

    o.handle:setRecvCb(function (data, from_addr, from_port, self)
        if not self.respCb then
            return
        end

        local function reportErr(code, msg)
            logger:error(msg)
            if self.errCb then
                self.errCb(code, msg, table.unpack(self.args))
            end
        end

        local function parse(msg)
            if not msg then
                reportErr(0xffff, "Receive a invalid message.")
                return nil
            end
            if not msg.data then
                reportErr(0xffff, "Not a response message")
                return nil
            end
            if msg.did ~= self.devid then
                reportErr(0xffff, "Not a match Device ID")
                return nil
            end
            local payload =  json.decode(self.encryption:decrypt(msg.data))
            if not payload then
                reportErr(0xffff, "Invalid payload")
                return nil
            end
            if payload.error then
                reportErr(payload.error.code, payload.error.message)
                return nil
            end
            if payload.id ~= self.reqid then
                reportErr(0xffff, "response id ~= request id")
                return nil
            end
            return payload.result
        end

        local result = parse(unpack(data))
        if not result then
            self.respCb = nil
            return
        end

        local cb = self.respCb
        self.respCb = nil

        cb(result, table.unpack(self.args))
    end, o)

    ---Start a request and ``respCb`` will be called when a response is received.
    ---@param respCb fun(result: any, ...): boolean
    ---@param method string
    ---@param params? any
    ---@return boolean status true on success, false on failure.
    function o:request(respCb, method, params)
        assert(type(respCb) == "function")

        if self.respCb then
            logger:error("The last request is in progress.")
            return false
        end

        self.respCb = respCb
        self.reqid = self.reqid + 1
        local data = {
            id = self.reqid,
            method = method,
            params = params or json.empty_array
        }
        local m = pack(0, self.devid, os.time() - self.stampDiff, nil, self.encryption:encrypt(json.encode(data)))
        if not self.handle:send(m) then
            logger:error("Failed to send message")
            return false
        end

        return true
    end

    function o:abort()
        self.respCb = nil
    end

    ---Set error callback.
    ---@param cb fun(code: integer, message: string, ...)
    function o:setErrCb(cb)
        self.errCb = cb
    end

    return o
end

return protocol
