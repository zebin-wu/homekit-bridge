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
        local m <close> = hash.md5()
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
        local md5 <close> = hash.md5()
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
        local md5 <close> = hash.md5()
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
---@return string package
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
---@param cb fun(addr: string, devid: integer, stamp: integer) Function call when the device is scaned.
---@param timeout integer Timeout period (in seconds).
---@param addr? string Target Address.
---@return boolean status true on success, false on failure.
function protocol.scan(cb, timeout, addr, ...)
    assert(type(cb) == "function")
    assert(math.type(timeout) == "integer")
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

    if not handle:sendto(packHello(), addr or "255.255.255.255", 54321) then
        logger:error("Failed to send hello message.")
        handle:close()
        return false
    end

    handle:setRecvCb(function (data, from_addr, from_port, context)
        if from_port ~= 54321 then
            return
        end
        local m = unpack(data)
        if m and m.unknown == 0 and m.data == nil then
            context.cb(from_addr, m.did, m.stamp, table.unpack(context.args))
            if context.addr and context.addr == from_addr then
                logger:debug("Scan done.")
                context.handle:close()
                context.timer:cancel()
            end
        end
    end, context)

    context.cb = cb
    context.handle = handle
    if addr then
        context.addr = addr
    end
    context.args = { ... }

    context.timer = timer.create(function(context)
        logger:debug("Scan done.")
        context.handle:close()
    end, context)
    context.timer:start(timeout * 1000)

    context.cb = cb
    context.handle = handle
    if addr then
        context.addr = addr
    end
    context.args = { ... }

    return true
end

---@class MiioError:table Error delivered by the target device.
---
---@field code integer Error code.
---@field message string Error message.

---Create a PCB(protocol control block).
---@param addr string Device address.
---@param devid integer Device ID: 32-bit.
---@param token string Device token: 128-bit.
---@param stamp integer Device time stamp obtained by scanning.
---@return PCB pcb Protocol control block.
function protocol.create(addr, devid, token, stamp)
    assert(type(addr) == "string")
    assert(math.type(devid) == "integer")
    assert(type(token) == "string")
    assert(#token == 16)
    assert(math.type(stamp) == "integer")

    ---@class PCB:table protocol control block.
    local pcb = {
        addr = addr,
        stampDiff = os.time() - stamp,
        token = token,
        devid = devid,
        reqid = 0
    }

    pcb.handle = udp.open("inet")
    if not pcb.handle then
        logger:error("Failed to open a UDP handle.")
        return nil
    end
    if not pcb.handle:connect(addr, 54321) then
        logger:error(("Failed to connect to %s:%d"):format(addr, 54321))
        pcb.handle:close()
        return nil
    end

    pcb.encryption = newEncryption(token)
    if not pcb.encryption then
        logger:error("Failed to new a encryption.")
        pcb.handle:close()
        return nil
    end

    ---Parse a message.
    ---@param self PCB
    ---@param msg MiioMessage
    ---@return MiioError|string|nil error
    ---@return any result
    local function parse(self, msg)
        if not msg then
            return "Receive a invalid message."
        end
        if msg.did ~= self.devid then
            return "Not a match Device ID."
        end
        if not msg.data then
            return "Not a response message."
        end
        local s = self.encryption:decrypt(msg.data)
        if not s then
            return "Failed to decrypt the message."
        end
        logger:debug(("%s <= %s:%d"):format(s, self.addr, 54321))
        local payload =  json.decode(s)
        if not payload then
            return "Failed to parse the JSON string."
        end
        if payload.id ~= self.reqid then
            return "response id ~= request id"
        end
        if payload.error then
            return payload.error
        end
        return nil, payload.result
    end

    pcb.handle:setRecvCb(function (data, from_addr, from_port, self)
        if not self.respCb then
            logger:debug("No pending request, skip the received message.")
            return
        end

        local err, result = parse(self, unpack(data, self.token))
        local cb = self.respCb
        local args = self.args
        self.respCb = nil
        self.args = nil
        cb(err, result, table.unpack(args))
    end, pcb)

    ---Start a request and ``respCb`` will be called when a response is received.
    ---@param respCb fun(err: MiioError|string|nil, result: any, ...) Response callback.
    ---@param method string The request method.
    ---@param params? table Array of parameters.
    function pcb:request(respCb, method, params, ...)
        assert(type(respCb) == "function")
        assert(type(method) == "string")

        if params then
            assert(type(params) == "table")
        end

        if self.respCb then
            logger:error("The previous request is in progress.")
            return false
        end

        self.respCb = respCb
        self.args = {...}
        self.reqid = self.reqid + 1
        local data = json.encode({
            id = self.reqid,
            method = method,
            params = params or json.empty_array
        })

        assert(self.handle:send(pack(0, self.devid, os.time() - self.stampDiff,
            self.token, self.encryption:encrypt(data))))
        logger:debug(("%s => %s:%d"):format(data, self.addr, 54321))
    end

    ---Abort the previous request.
    function pcb:abort()
        self.respCb = nil
    end

    ---Destroy the PCB.
    function pcb:destroy()
        self.handle:close()
    end

    return pcb
end

return protocol
