local hash = require "hash"
local cipher = require "cipher"
local json = require "cjson"
local transport = require "miio.transport"

local assert = assert
local pcall = pcall
local type = type
local error = error
local floor = math.floor
local random = math.random
local tointeger = math.tointeger
local spack = string.pack
local sunpack = string.unpack
local schar = string.char
local srep = string.rep
local tconcat = table.concat

local M = {}
local logger = log.getLogger("miio.protocol")

---@class MiioProtocolRuntime
---@field transport MiioTransport?
---@field virtualDid integer?
---@field _reqid integer
local runtime = {}

---
--- Message format
---
--- 0                   1                   2                   3
--- 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
--- +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--- | Magic number = 0x2131         | Packet Length (incl. header)  |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Device ID ("did"), high 32 bits                               |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Device ID ("did"), low 32 bits                                |
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
---  Device ID: 64 bits
---      Unique number. Possibly derived from the MAC address.
---      except in the "Hello" packet, when it's 0xFFFFFFFFFFFFFFFF
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
---
--- miIO Encryption.
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
local encryption = {}

---Encrypt data.
---@param input string
---@return string output
function encryption:encrypt(input)
    return self.ctx:process("encrypt", self.key, self.iv, input)
end

---Decrypt data.
---@param input string
---@return string output
function encryption:decrypt(input)
    return self.ctx:process("decrypt", self.key, self.iv, input)
end

---Calculates a MD5 checksum for the given data.
---@param ... string
---@return string digest
local function md5(...)
    local ctx = hash.create("MD5")
    for i = 1, select("#", ...) do
        local part = select(i, ...)
        if part ~= nil and part ~= "" then
            ctx:update(part)
        end
    end
    return ctx:digest()
end

---Create an encryption.
---@param token string Device token.
---@return MiioEncryption encryption A new encryption.
---@nodiscard
local function createEncryption(token)
    local ctx = cipher.create("AES-128-CBC")
    ctx:setPadding("PKCS7")

    local key = md5(token)
    local iv = md5(key, token)

    ---@class MiioEncryption
    local o = {
        ctx = ctx,
        key = key,
        iv = iv,
    }

    return setmetatable(o, {
        __index = encryption
    })
end

---Create a virtual device ID for probe packets.
---@return integer did
---@nodiscard
local function createVirtualDid()
    local now = floor(core.time())
    local high = floor(now / 1000) & 0xffffffff
    local low = (now & 0xffffffff) ~ random(0, 0xffffffff)
    return (high << 32) | low
end

---@param netifs? string[]
---@param virtualDid? integer
---@return string[]? netifs
---@return integer? virtualDid
local function normalizeCreateArgs(netifs, virtualDid)
    assert(netifs == nil or type(netifs) == "table", "netifs must be a table")
    if virtualDid ~= nil then
        virtualDid = assert(tointeger(virtualDid), "virtualDid must be an integer")
    end
    return netifs, virtualDid
end

---@param self MiioProtocolRuntime
---@return integer reqid
local function nextRequestId(self)
    local reqid = self._reqid + 1
    if reqid > 9999 then
        reqid = 1
    end
    self._reqid = reqid
    return reqid
end

---Pack a message to a binary package.
---@param did integer Device ID: 64-bit.
---@param stamp integer Stamp: 32 bit unsigned int.
---@param token? string Device token: 128-bit.
---@param data? string Optional variable-sized data.
---@return string package
---@nodiscard
local function pack(did, stamp, token, data)
    local len = 32
    if data then
        len = len + #data
    end

    local header = spack(">I2>I2>I8>I4",
        0x2131, len, did, stamp)
    local checksum = nil
    if token then
        checksum = md5(header, token, data or "")
    else
        checksum = srep(schar(0xff), 16)
    end
    assert(#checksum == 16)

    return header .. checksum .. (data or "")
end

---Pack a probe packet.
---
---The probe packet keeps the 32-byte miIO header layout, but replaces the
---MD5/token area with `MDID + virtualDid + 0x00000000`.
---@param did integer Virtual device ID: 64-bit.
---@return string package
---@nodiscard
local function packProbe(did)
    return tconcat({
        spack(">I2I2I8I4", 0x2131, 32, -1, 0xffffffff),
        "MDID",
        spack(">I8I4", did, 0),
    })
end

---Unpack a message from a binary package.
---@param package string A binary package.
---@param token? string Device token: 128-bit.
---@return integer did
---@return integer stamp
---@return string? data
---@nodiscard
local function unpack(package, token)
    if sunpack(">I2", package, 1) ~= 0x2131 then
        error("Invalid magic number.")
    end

    local len = sunpack(">I2", package, 3)
    if len < 32 or len ~= #package then
        error("Invalid package length.")
    end

    local data = nil
    if len > 32 then
        data = sunpack("c" .. len - 32, package, 33)
    end

    if token then
        local checksum = md5(sunpack("c16", package, 1), token, data or "")
        assert(#checksum == 16)
        if checksum ~= sunpack("c16", package, 17) then
            error("Got checksum error which indicates use of an invalid token.")
        end
    end

    return sunpack(">I8", package, 5),
        sunpack(">I4", package, 13),
        data
end

---@param queueSize integer
---@param currentTp MiioTransport
---@param matcher fun(packet:string, addr:string, port:integer, ifname:string):any?
---@return MessageQueue queue
---@return fun() close
local function createReceiver(queueSize, currentTp, matcher)
    assert(currentTp, "transport not inited")

    local queue = core.createMQ(queueSize)
    local subId = currentTp:subscribe(function(packet, addr, port, ifname)
        local result = matcher(packet, addr, port, ifname)
        if result ~= nil then
            queue:send(result)
        end
    end)
    local closed = false

    return queue, function()
        if closed then
            return
        end
        closed = true
        currentTp:unsubscribe(subId)
    end
end

---@param queueSize integer
---@param currentTp MiioTransport
---@param matcher fun(packet:string, addr:string, port:integer, ifname:string):any?
---@param action fun(queue:MessageQueue):any
---@return any result
local function withReceiver(queueSize, currentTp, matcher, action)
    local queue, close = createReceiver(queueSize, currentTp, matcher)
    local ok, result = pcall(action, queue)
    close()
    if not ok then
        error(result)
    end
    return result
end

---@class ScanResult Scan Result.
---
---@field addr string Device address.
---@field devid integer Device ID: 64-bit.
---@field ifname string Network interface name.
---@field stamp integer Device time stamp.

---Scan for devices in the local network.
---
---This method is used to discover supported devices by sending
---a probe message to the broadcast address on port 54321.
---If the target IP address is given, the probe will be send as an unicast packet.
---@param self MiioProtocolRuntime
---@param timeout integer Timeout period (in milliseconds).
---@param addr? string Target Address.
---@return ScanResult[] results A array of scan results.
---@nodiscard
function runtime:scan(timeout, addr)
    assert(timeout > 0, "timeout must be greater then 0")
    local currentTp = assert(self.transport, "protocol closed")
    local virtualDid = assert(self.virtualDid, "protocol closed")

    local numSend = 1
    local probe = packProbe(virtualDid)

    if not addr then
        numSend = 3
    end

    local seen = {}
    local deadline = floor(core.time()) + timeout
    return withReceiver(64, currentTp, function(packet, fromAddr, _, ifname)
        if addr ~= nil and fromAddr ~= addr then
            return
        end
        local success, did, stamp, data = pcall(unpack, packet)
        if not success or did == -1 or data ~= nil or seen[fromAddr] then
            return
        end
        seen[fromAddr] = true
        return {
            addr = fromAddr,
            devid = did,
            ifname = ifname,
            stamp = stamp,
        }
    end, function(queue)
        local results = {}
        for _ = 1, numSend do
            assert(currentTp:sendto(probe, addr or "255.255.255.255", 54321) > 0, "failed to send probe message")
        end

        while true do
            local ok, result = queue:recvUntil(deadline)
            if not ok then
                break
            end
            results[#results + 1] = result
            if addr then
                break
            end
        end
        return results
    end)
end

---@class MiioPcb: table miio protocol control block.
---@field runtime MiioProtocolRuntime
local pcb = {}

---@class MiioError miIO error.
---
---@field code integer Error code.
---@field message string Error message.

---@param self MiioPcb
---@param raw string
---@param reqid integer
---@return any? decoded
local function decodeResponse(self, raw, reqid)
    local success, did, _, data = pcall(unpack, raw, self.token)
    if not success or did ~= self.devid or data == nil then
        return
    end

    local ok, payload = pcall(self.encryption.decrypt, self.encryption, data)
    if not ok or payload == nil then
        return
    end

    local decodedOk, decoded = pcall(json.decode, payload)
    if not decodedOk or decoded == nil or decoded.id ~= reqid then
        return
    end

    return decoded
end

---Handshake.
---@param timeout integer Timeout period (in milliseconds).
function pcb:handshake(timeout)
    logger:debug("Handshake ...")
    local results = self.runtime:scan(timeout, self.addr)
    local result = results[1]
    assert(result ~= nil, "handshake failed")
    logger:debug("Handshake done.")
    self.devid = result.devid
    self.ifname = result.ifname
    self.stampDiff = floor(core.time() / 1000) - result.stamp
end

---Start a request.
---@param timeout integer Timeout period (in milliseconds).
---@param method string The request method.
---@param ... any The request parameters.
---@return any result
function pcb:request(timeout, method, ...)
    assert(timeout > 0, "timeout must be greater then 0")
    assert(type(method) == "string")

    local params = {...}
    if #params == 0 then
        params = nil
    end

    if self.stampDiff == nil then
        self:handshake(timeout)
    end

    local reqid = nextRequestId(self.runtime)

    local plain = json.encode({
        id = reqid,
        method = method,
        params = params
    })
    local packet = pack(
        self.devid,
        floor(core.time() / 1000) - self.stampDiff,
        self.token,
        self.encryption:encrypt(plain)
    )
    local currentTp = assert(self.runtime.transport, "protocol closed")
    local deadline = floor(core.time()) + timeout
    logger:debug(("%s => %s"):format(plain, self.addr))
    local result = withReceiver(1, currentTp, function(raw, fromAddr, _, ifname)
        if fromAddr ~= self.addr then
            return
        end
        local decoded = decodeResponse(self, raw, reqid)
        if decoded == nil then
            return
        end
        self.ifname = ifname
        return decoded
    end, function(queue)
        local sent = 0
        if self.ifname ~= nil then
            local ok, sendResult = pcall(currentTp.sendto, currentTp, packet, self.addr, 54321, self.ifname)
            if ok then
                sent = sendResult
            else
                logger:debug(("send via %s failed, retry all netifs, %s"):format(self.ifname, tostring(sendResult)))
                self.ifname = nil
            end
        end
        if sent == 0 then
            sent = currentTp:sendto(packet, self.addr, 54321)
        end
        if sent == 0 then
            error("failed to send request")
        end

        local ok, recvResult = queue:recvUntil(deadline)
        if not ok then
            self.ifname = nil
            self.stampDiff = nil
            error(recvResult)
        end
        return recvResult
    end)

    logger:debug(("%s => %s"):format(self.addr, json.encode(result)))
    ---@class MiioError
    local err = result.error
    if err then
        error(err)
    end

    return result.result
end

---Create a PCB(protocol control block).
---@param self MiioProtocolRuntime
---@param addr string Device address.
---@param token string Device token: 128-bit.
---@return MiioPcb pcb Protocol control block.
---@nodiscard
function runtime:createPcb(addr, token)
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)
    assert(self.transport ~= nil, "protocol closed")

    ---@class MiioPcb
    local o = {
        addr = addr,
        runtime = self,
        token = token,
    }

    o.encryption = createEncryption(token)

    setmetatable(o, {
        __index = pcb
    })

    return o
end

---Close the miIO protocol runtime.
---@param self MiioProtocolRuntime
function runtime:close()
    local currentTp = self.transport
    if currentTp ~= nil then
        currentTp:close()
        self.transport = nil
    end
    self.virtualDid = nil
end

---Create a miIO protocol runtime.
---@param netifs? string[] Network interface names.
---@param virtualDid? integer Virtual device ID: 64-bit.
---@return MiioProtocolRuntime runtime
---@nodiscard
function M.create(netifs, virtualDid)
    netifs, virtualDid = normalizeCreateArgs(netifs, virtualDid)

    ---@class MiioProtocolRuntime
    local o = {
        transport = transport.create(netifs),
        virtualDid = virtualDid or createVirtualDid(),
        _reqid = 0,
    }

    return setmetatable(o, {
        __index = runtime
    })
end

return M
