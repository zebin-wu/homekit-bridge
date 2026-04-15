local socket = require "socket"
local hash = require "hash"
local cipher = require "cipher"
local json = require "cjson"

local assert = assert
local type = type
local error = error
local floor = math.floor
local spack = string.pack
local sunpack = string.unpack
local schar = string.char
local srep = string.rep
local tconcat = table.concat

local M = {}
local logger = log.getLogger("miio.protocol")

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

    return tconcat({header, checksum, data or ""})
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

---@class ScanResult Scan Result.
---
---@field addr string Device address.
---@field devid integer Device ID: 64-bit.
---@field stamp integer Device time stamp.

---Scan for devices in the local network.
---
---This method is used to discover supported devices by sending
---a handshake message to the broadcast address on port 54321.
---If the target IP address is given, the handshake will be send as an unicast packet.
---@param timeout integer Timeout period (in milliseconds).
---@param addr? string Target Address.
---@return ScanResult[] results A array of scan results.
---@nodiscard
function M.scan(timeout, addr)
    assert(timeout > 0, "timeout must be greater then 0")

    local numSend = 1
    local sock <close> = socket.create("UDP", "IPV4")
    sock:settimeout(timeout)

    if not addr then
        numSend = 3
        sock:enablebroadcast()
    end

    local hello = pack(-1, -1)
    for _ = 1, numSend do
        assert(sock:sendto(hello, addr or "255.255.255.255", 54321), "failed to send hello message")
    end

    local seen = {}
    local results = {}

    while true do
        local success, result, fromAddr, _ = pcall(sock.recvfrom, sock, 1024)
        if success == false then
            if addr == nil and result:find("timeout") then
                return results
            end
            error(result)
        end
        local did, stamp, data = unpack(result)
        if did == -1 or data then
            goto continue
        end
        if not seen[fromAddr] then
            table.insert(results, {
                addr = fromAddr,
                devid = did,
                stamp = stamp
            })
            if addr then
                return results
            end
            seen[fromAddr] = true
        end
::continue::
    end
end

---@class MiioPcb: table miio protocol control block.
local pcb = {}

---@class MiioError miIO error.
---
---@field code integer Error code.
---@field message string Error message.

---Handshake.
---@param timeout integer Timeout period (in milliseconds).
function pcb:handshake(timeout)
    logger:debug("Handshake ...")
    local results = M.scan(timeout, self.addr)
    local result = results[1]
    logger:debug("Handshake done.")
    self.devid = result.devid
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

    local sock <close> = socket.create("UDP", "IPV4")
    sock:settimeout(timeout)
    sock:connect(self.addr, 54321)

    local reqid = self.reqid + 1
    if reqid == 9999 then
        reqid = 1
    end
    self.reqid = reqid
    do
        local data = json.encode({
            id = reqid,
            method = method,
            params = params
        })

        sock:send(pack(self.devid, floor(core.time() / 1000) - self.stampDiff,
            self.token, self.encryption:encrypt(data)))

        logger:debug(("%s => %s"):format(data, self.addr))
    end

    local success, result = pcall(sock.recv, sock, 1024)
    if success == false then
        if result:find("timeout") then
            self.stampDiff = nil
        end
        error(result)
    end
    self.errCnt = 0
    local did, _, data = unpack(result, self.token)

    if did ~= self.devid or data == nil then
        error("Receive a invalid message.")
    end
    local s = self.encryption:decrypt(data)
    if not s then
        error("Failed to decrypt the message.")
    end
    logger:debug(("%s => %s"):format(self.addr, s))
    local payload =  json.decode(s)
    if not payload then
        error("Failed to parse the JSON string.")
    end
    if payload.id ~= reqid then
        error("response id ~= request id")
    end
    ---@class MiioError
    local err = payload.error
    if err then
        error(err)
    end

    return payload.result
end

---Create a PCB(protocol control block).
---@param addr string Device address.
---@param token string Device token: 128-bit.
---@return MiioPcb pcb Protocol control block.
---@nodiscard
function M.create(addr, token)
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class MiioPcb
    local o = {
        addr = addr,
        token = token,
        reqid = 0,
    }

    o.encryption = createEncryption(token)

    setmetatable(o, {
        __index = pcb
    })

    return o
end

return M
