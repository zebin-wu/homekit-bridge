local udp = require "udp"
local timer = require "timer"
local hash = require "hash"
local json = require "cjson"
local ErrorCode = require "miio.error".code

local assert = assert
local type = type

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
---      Unix style epoch time
---
---  MD5 checksum:
---      calculated for the whole packet including the MD5 field itself,
---      which must be initialized with device token.
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
---@field data string

---
--- Encryption
---
--- The variable-sized data payload is encrypted with the Advanced Encryption Standard (AES).
---
--- A 128-bit key and Initialization Vector are both derived from the Token as follows:
---   Key = MD5(Token)
---   IV  = MD5(Key + Token)
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
        m:update(data)
        return m:digest()
    end

    local cipher = require("cipher").create("AES-128-CBC")
    if not cipher then
        logger:error("Failed to create a AES-128-CBC cipher.")
        return nil
    end

    if not cipher:setPadding("PKCS7") then
        logger:error("Failed to set padding to the cipher.")
        return nil
    end

    local key = md5(token)
    local iv = md5(key .. token)

    local o = {
        cipher = cipher,
        key = key,
        iv = iv,
    }

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
---@param token? string Device token: 128-bit.
---@param data? string Optional variable-sized data.
---@return string package
local function pack(unknown, did, stamp, token, data)
    local len = 32
    if data then
        len = len + #data
    end

    local header = string.pack(">I2>I2>I4>I4>I4",
        0x2131, len, unknown, did, stamp)
    local checksum = nil
    if token then
        local md5 = hash.md5()
        md5:update(header .. token)
        if data then
            md5:update(data)
        end
        checksum = md5:digest()
    else
        checksum = string.pack(">I4>I4>I4>I4",
            0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff)
    end
    assert(#checksum == 16)

    local msg = header .. checksum
    if data then
        msg = msg .. data
    end

    return msg
end

---Unpack a message from a binary package.
---@param package string A binary package.
---@param token? string Device token: 128-bit.
---@return MiioMessage|nil message
local function unpack(package, token)
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

    if token then
        local md5 = hash.md5()
        md5:update(string.unpack("c16", package, 1) .. token)
        if data then
            md5:update(data)
        end
        if md5:digest() ~= string.unpack("c16", package, 17) then
            logger:error("Got checksum error which indicates use of an invalid token.")
            return nil
        end
    end

    return {
        unknown = string.unpack(">I4", package, 5),
        did = string.unpack(">I4", package, 9),
        stamp = string.unpack(">I4", package, 13),
        data = data
    }
end

---Pack a hello package.
---@return string package A binary package.
local function packHello()
    return pack(
        0xffffffff,
        0xffffffff,
        0xffffffff
    )
end

---Scan for devices in the local network.
---
---This method is used to discover supported devices by sending
---a handshake message to the broadcast address on port 54321.
---If the target IP address is given, the handshake will be send as an unicast packet.
---@param cb fun(addr: string, devid: integer, stamp: integer, ...) Function call when the device is scaned.
---@param addr? string Target Address.
---@return MiioScanContext|nil ctx Scan context.
function protocol.scan(cb, addr, ...)
    assert(type(cb) == "function")

    local handle = udp.open("inet")
    if not handle then
        logger:error("Failed to open a UDP handle.")
        return nil
    end

    if not addr then
        handle:enableBroadcast()
    end

    local hello = packHello()
    for i = 1, 3, 1 do
        if not handle:sendto(hello, addr or "255.255.255.255", 54321) then
            logger:error("Failed to send hello message.")
            handle:close()
            return nil
        end
    end

    ---@class MiioScanContext
    local ctx = {
        cb = cb,
        handle = handle,
        addr = addr,
        seen_addr = {},
        args = {...}
    }

    handle:setRecvCb(function (data, from_addr, from_port, self)
        if from_port ~= 54321 then
            return
        end
        if self.seen_addr[from_addr] then
            return
        end
        self.seen_addr[from_addr] = true
        local m = unpack(data)
        if m and m.unknown == 0 and m.data == nil then
            if self.addr and self.addr == from_addr then
                logger:debug("Scan done.")
                self.handle:close()
            end
            self.cb(from_addr, m.did, m.stamp, table.unpack(self.args))
        end
    end, ctx)

    ---Stop scanning.
    function ctx:stop()
        self.handle:close()
    end

    return ctx
end

---@class MiioPcb: MiioPcbPriv miio protocol control block.
local _pcb = {}

---Start a request and ``respCb`` will be called when a response is received.
---@param respCb fun(result: any, ...) Response callback.
---@param errCb fun(code: integer, message: string, ...) Error Callback.
---@param timeout integer Timeout period (in milliseconds).
---@param method string The request method.
---@param params? table Array of parameters.
function _pcb:request(respCb, errCb, timeout, method, params, ...)
    assert(self.respCb == nil, "previous request is in progress")
    assert(type(respCb) == "function")
    assert(type(errCb) == "function")
    assert(timeout > 0, "timeout must be greater then 0")
    assert(type(method) == "string")

    if params then
        assert(type(params) == "table")
    end

    self.respCb = respCb
    self.errCb = errCb
    self.args = {...}

    local reqid = self.reqid + 1
    if reqid == 9999 then
        reqid = 1
    end
    self.reqid = reqid

    local data = json.encode({
        id = reqid,
        method = method,
        params = params or nil
    })

    assert(self.handle:send(pack(0, self.devid, os.time() - self.stampDiff,
        self.token, self.encryption:encrypt(data))))

    self.handle:setRecvCb(function (data, from_addr, from_port, self)
        if not self.respCb then
            self.logger:debug("No pending request, skip the received message.")
            return
        end

        self.timer:stop()
        self.handle:setRecvCb()

        local respCb = self.respCb
        local errCb = self.errCb
        local args = self.args
        self.respCb = nil
        self.errCb = nil
        self.args = nil

        local msg = unpack(data, self.token)

        if not msg then
            errCb(ErrorCode.InvalidData, "Receive a invalid message.", table.unpack(args))
            return
        end
        if msg.did ~= self.devid then
            errCb(ErrorCode.InvalidData, "Not a match Device ID.", table.unpack(args))
            return
        end
        if not msg.data then
            errCb(ErrorCode.InvalidData, "Not a response message.", table.unpack(args))
            return
        end
        local s = self.encryption:decrypt(msg.data)
        if not s then
            errCb(ErrorCode.InvalidData, "Failed to decrypt the message.", table.unpack(args))
            return
        end
        self.logger:debug("=> " .. s)
        local payload =  json.decode(s)
        if not payload then
            errCb(ErrorCode.InvalidData, "Failed to parse the JSON string.", table.unpack(args))
            return
        end
        if payload.id ~= self.reqid then
            errCb(ErrorCode.InvalidData, "response id ~= request id", table.unpack(args))
            return
        end
        local error = payload.error
        if error then
            errCb(error.code, error.message, table.unpack(args))
            return
        end

        respCb(payload.result, table.unpack(args))
    end, self)

    self.timer:start(timeout)
    self.logger:debug("<= " .. data)
end

---Abort the previous request.
function _pcb:abort()
    if self.respCb then
        self.respCb = nil
        self.errCb = nil
        self.args = nil
        self.timer:stop()
        self.handle:setRecvCb()
    end
end

---Create a PCB(protocol control block).
---@param addr string Device address.
---@param devid integer Device ID: 32-bit.
---@param token string Device token: 128-bit.
---@param stamp integer Device time stamp obtained by scanning.
---@return MiioPcb pcb Protocol control block.
function protocol.create(addr, devid, token, stamp)
    assert(type(addr) == "string")
    assert(math.type(devid) == "integer")
    assert(type(token) == "string")
    assert(#token == 16)
    assert(math.type(stamp) == "integer")

    ---@class MiioPcbPriv: table
    local pcb = {
        logger = log.getLogger("miio.protocol:" .. addr),
        stampDiff = os.time() - stamp,
        token = token,
        devid = devid,
        reqid = 0
    }

    local handle = udp.open("inet")
    if not handle then
        pcb.logger:error("Failed to open a UDP handle.")
        handle:close()
        return nil
    end
    if not handle:connect(addr, 54321) then
        pcb.logger:error(("Failed to connect to '%s'."):format(addr))
        handle:close()
        return nil
    end

    pcb.encryption = newEncryption(token)
    if not pcb.encryption then
        pcb.logger:error("Failed to new a encryption.")
        return nil
    end

    pcb.handle = handle
    pcb.timer = timer.create(function (self)
        self.handle:setRecvCb()
        local errCb = self.errCb
        local args = self.args
        self.respCb = nil
        self.errCb = nil
        self.args = nil
        errCb(ErrorCode.Timeout, "Request timeout.", table.unpack(args))
    end, pcb)

    setmetatable(pcb, {
        __index = _pcb
    })

    return pcb
end

return protocol
