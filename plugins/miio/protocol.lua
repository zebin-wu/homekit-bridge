local socket = require "socket"
local netiflib = require "netif"
local hash = require "hash"
local cipher = require "cipher"
local json = require "cjson"

local assert = assert
local error = error
local floor = math.floor
local ipairs = ipairs
local pairs = pairs
local pcall = pcall
local random = math.random
local tointeger = math.tointeger
local tostring = tostring
local type = type
local xpcall = xpcall
local spack = string.pack
local sunpack = string.unpack
local schar = string.char
local srep = string.rep
local tconcat = table.concat
local traceback = debug.traceback

local M = {}
local logger = log.getLogger("miio.protocol")

local UDP_PORT = 54321
local MAX_MSG_LEN = 2048
local SCAN_ANY_ADDR = "*"

---@class MiioProtocolSocket
---@field ifname string
---@field sock Socket
---@field reader Timer

---@class MiioScanWaiter
---@field mq MessageQueue?

---@class MiioRequestWaiter
---@field mq MessageQueue?
---@field devid integer
---@field reqid integer
---@field token string
---@field encryption MiioEncryption

---@class MiioProtocolRuntime
---@field sockets table<string, MiioProtocolSocket>
---@field localPort integer
---@field running boolean
---@field scanMqs table<string, MiioScanWaiter[]>
---@field requestMqs table<string, MiioRequestWaiter[]>
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

local function isUsableNetif(netif)
    if not netiflib.isUp(netif) then
        return false
    end
    local addr = netiflib.getIpv4Addr(netif)
    return addr ~= "0.0.0.0" and addr ~= "127.0.0.1"
end

---@param netifs? string[]
---@return string[] results
local function resolveNetifs(netifs)
    if netifs ~= nil then
        assert(type(netifs) == "table", "netifs must be a table")
    end

    local available = {}
    local results = {}
    for _, netif in ipairs(netiflib.getInterfaces()) do
        local ifname = netiflib.getName(netif)
        local usable = isUsableNetif(netif)
        available[ifname] = usable
        if netifs == nil and usable then
            results[#results + 1] = ifname
        end
    end

    if netifs ~= nil then
        local seen = {}
        for i, ifname in ipairs(netifs) do
            assert(type(ifname) == "string", ("network interface #%d must be a string"):format(i))
            if not seen[ifname] then
                local usable = available[ifname]
                if usable == nil then
                    error(("network interface #%d not found"):format(i))
                end
                if not usable then
                    error(("network interface #%d is not usable"):format(i))
                end
                results[#results + 1] = ifname
                seen[ifname] = true
            end
        end
    end

    assert(#results > 0, "no available IPv4 network interface")
    return results
end

local function destroySocket(ctx)
    if ctx == nil or ctx.sock == nil then
        return
    end
    if ctx.reader ~= nil then
        ctx.reader:stop()
        ctx.reader = nil
    end
    pcall(ctx.sock.destroy, ctx.sock)
    ctx.sock = nil
end

---@param netifs string[]
---@return integer localPort
---@return table<string, MiioProtocolSocket> sockets
local function createSockets(netifs)
    local sockets = {}
    local localPort = nil
    local failures = {}

    for _, ifname in ipairs(netifs) do
        local sock = socket.create("UDP", "IPV4")
        local addr
        local success, result = pcall(function()
            sock:settimeout(0)
            sock:enablebroadcast()
            sock:reuseaddr()
            sock:bindif(ifname)
            sock:bind("0.0.0.0", localPort or 0)
            if localPort == nil then
                local _, port = sock:getsockname()
                localPort = port
            end
            addr = netiflib.getIpv4Addr(netiflib.find(ifname))
        end)
        if success then
            sockets[ifname] = {
                ifname = ifname,
                sock = sock,
            }
            logger:info(("created socket, %s, %s, %s"):format(ifname, addr, localPort))
        else
            logger:error(("create socket error, %s, %s"):format(ifname, tostring(result)))
            failures[#failures + 1] = ("%s: %s"):format(ifname, tostring(result))
            pcall(sock.destroy, sock)
        end
    end

    if localPort ~= nil and #failures == 0 then
        return localPort, sockets
    end

    for _, ctx in pairs(sockets) do
        destroySocket(ctx)
    end
    if #failures > 0 then
        error("failed to bind miio transport sockets: " .. tconcat(failures, "; "))
    end
    error("failed to bind miio transport sockets")
end

local function snapshotWaiters(waiters)
    if waiters == nil or #waiters == 0 then
        return nil
    end
    local snapshot = {}
    for i = 1, #waiters do
        snapshot[i] = waiters[i]
    end
    return snapshot
end

local function appendWaiter(waitersByKey, key, waiter)
    local waiters = waitersByKey[key]
    if waiters == nil then
        waiters = {}
        waitersByKey[key] = waiters
    end
    waiters[#waiters + 1] = waiter
end

local function removeWaiter(waitersByKey, key, waiter)
    local waiters = waitersByKey[key]
    if waiters == nil then
        return
    end
    for i = #waiters, 1, -1 do
        if waiters[i] == waiter then
            waiters[i] = waiters[#waiters]
            waiters[#waiters] = nil
            break
        end
    end
    if #waiters == 0 then
        waitersByKey[key] = nil
    end
end

local function sendWaiter(waiter, ...)
    local mq = waiter.mq
    if mq == nil then
        return
    end
    local ok, err = pcall(mq.send, mq, ...)
    if not ok then
        logger:debug(("drop miio packet: %s"):format(tostring(err)))
    end
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
---@return string[] netifs
---@return integer? virtualDid
local function normalizeCreateArgs(netifs, virtualDid)
    if virtualDid ~= nil then
        virtualDid = assert(tointeger(virtualDid), "virtualDid must be an integer")
    end
    return resolveNetifs(netifs), virtualDid
end

local function hasPendingRequest(self, reqid)
    for _, waiters in pairs(self.requestMqs) do
        for i = 1, #waiters do
            if waiters[i].reqid == reqid then
                return true
            end
        end
    end
    return false
end

---@param self MiioProtocolRuntime
---@return integer reqid
local function nextRequestId(self)
    local reqid = self._reqid
    for _ = 1, 9999 do
        reqid = reqid + 1
        if reqid > 9999 then
            reqid = 1
        end
        if not hasPendingRequest(self, reqid) then
            self._reqid = reqid
            return reqid
        end
    end
    error("too many pending requests")
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

---@class ScanResult Scan Result.
---
---@field addr string Device address.
---@field devid integer Device ID: 64-bit.
---@field ifname string Network interface name.
---@field stamp integer Device time stamp.

local function receivePacket(ctx)
    local success, data, addr, port = xpcall(ctx.sock.recvfrom, traceback, ctx.sock, MAX_MSG_LEN)
    if not success then
        return nil, data
    end
    if port ~= UDP_PORT then
        return false
    end
    return true, data, addr, port
end

---@param self MiioProtocolRuntime
---@param packet string
---@param addr string
---@param ifname string
local function dispatchScanPacket(self, packet, addr, ifname)
    local success, did, stamp, data = pcall(unpack, packet)
    if not success or did == -1 or data ~= nil then
        return false
    end

    ---@type ScanResult
    local result = {
        addr = addr,
        devid = did,
        ifname = ifname,
        stamp = stamp,
    }

    local waiters = snapshotWaiters(self.scanMqs[addr])
    if waiters ~= nil then
        for i = 1, #waiters do
            sendWaiter(waiters[i], result)
        end
    end

    if addr ~= SCAN_ANY_ADDR then
        waiters = snapshotWaiters(self.scanMqs[SCAN_ANY_ADDR])
        if waiters ~= nil then
            for i = 1, #waiters do
                sendWaiter(waiters[i], result)
            end
        end
    end

    return true
end

---@param waiter MiioRequestWaiter
---@param packet string
---@return any? decoded
local function decodeResponse(waiter, packet)
    local success, did, _, data = pcall(unpack, packet, waiter.token)
    if not success or did ~= waiter.devid or data == nil then
        return
    end

    local ok, payload = pcall(waiter.encryption.decrypt, waiter.encryption, data)
    if not ok or payload == nil then
        return
    end

    local decodedOk, decoded = pcall(json.decode, payload)
    if not decodedOk or decoded == nil or decoded.id ~= waiter.reqid then
        return
    end

    return decoded
end

---@param self MiioProtocolRuntime
---@param packet string
---@param addr string
---@param ifname string
local function dispatchRequestPacket(self, packet, addr, ifname)
    local waiters = self.requestMqs[addr]
    if waiters == nil then
        return
    end

    for i = 1, #waiters do
        local waiter = waiters[i]
        local decoded = decodeResponse(waiter, packet)
        if decoded ~= nil then
            sendWaiter(waiter, decoded, ifname)
            return
        end
    end
end

---@param self MiioProtocolRuntime
---@param packet string
---@param addr string
---@param ifname string
local function dispatchPacket(self, packet, addr, ifname)
    if dispatchScanPacket(self, packet, addr, ifname) then
        return
    end
    dispatchRequestPacket(self, packet, addr, ifname)
end

local function receiveLoop(self, ctx)
    while self.running and self.sockets[ctx.ifname] == ctx do
        local ok, data, addr = receivePacket(ctx)
        if ok == nil then
            if not self.running then
                return
            end
            logger:error(("recvfrom failed, %s, %s"):format(ctx.ifname, tostring(data)))
            return
        end
        if ok then
            dispatchPacket(self, data, addr, ctx.ifname)
        end
    end
end

---@param self MiioProtocolRuntime
---@param data string
---@param addr string
---@param port integer
---@param ifname? string
---@return integer sent
local function sendto(self, data, addr, port, ifname)
    assert(type(data) == "string")
    assert(type(addr) == "string")
    assert(type(port) == "number")

    local sent = 0

    if ifname ~= nil then
        local ctx = assert(self.sockets[ifname], ("unknown netif '%s'"):format(ifname))
        local success, result = pcall(ctx.sock.sendto, ctx.sock, data, addr, port)
        if not success then
            error(result)
        end
        return 1
    end

    for name, ctx in pairs(self.sockets) do
        local success, result = pcall(ctx.sock.sendto, ctx.sock, data, addr, port)
        if success then
            sent = sent + 1
        else
            logger:debug(("sendto failed, %s, %s"):format(name, tostring(result)))
        end
    end

    return sent
end

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
    assert(self.virtualDid ~= nil, "protocol closed")

    local key = addr or SCAN_ANY_ADDR
    local waiter = {
        mq = core.createMQ(64),
    }
    appendWaiter(self.scanMqs, key, waiter)

    local ok, result = pcall(function()
        local numSend = addr == nil and 3 or 1
        local probe = packProbe(self.virtualDid)
        local deadline = floor(core.time()) + timeout
        local seen = {}
        local results = {}

        for _ = 1, numSend do
            assert(sendto(self, probe, addr or "255.255.255.255", UDP_PORT) > 0, "failed to send probe message")
        end

        while true do
            local recvOk, item = waiter.mq:recvUntil(deadline)
            if not recvOk then
                break
            end
            if not seen[item.addr] then
                seen[item.addr] = true
                results[#results + 1] = item
                if addr ~= nil then
                    break
                end
            end
        end

        return results
    end)

    removeWaiter(self.scanMqs, key, waiter)
    waiter.mq = nil
    if not ok then
        error(result)
    end
    return result
end

---@class MiioPcb: table miio protocol control block.
---@field runtime MiioProtocolRuntime
local pcb = {}

---@class MiioError miIO error.
---
---@field code integer Error code.
---@field message string Error message.

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
    local waiter = {
        mq = core.createMQ(1),
        devid = self.devid,
        reqid = reqid,
        token = self.token,
        encryption = self.encryption,
    }
    appendWaiter(self.runtime.requestMqs, self.addr, waiter)

    logger:debug(("%s => %s"):format(plain, self.addr))
    local ok, result = pcall(function()
        local sent = 0
        local deadline = floor(core.time()) + timeout

        if self.ifname ~= nil then
            local sendOk, sendResult = pcall(sendto, self.runtime, packet, self.addr, UDP_PORT, self.ifname)
            if sendOk then
                sent = sendResult
            else
                logger:debug(("send via %s failed, retry all netifs, %s"):format(self.ifname, tostring(sendResult)))
                self.ifname = nil
            end
        end
        if sent == 0 then
            sent = sendto(self.runtime, packet, self.addr, UDP_PORT)
        end
        if sent == 0 then
            error("failed to send request")
        end

        local recvOk, recvResult, ifname = waiter.mq:recvUntil(deadline)
        if not recvOk then
            self.ifname = nil
            self.stampDiff = nil
            error(recvResult)
        end
        self.ifname = ifname
        return recvResult
    end)

    removeWaiter(self.runtime.requestMqs, self.addr, waiter)
    waiter.mq = nil
    if not ok then
        error(result)
    end

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
    assert(self.virtualDid ~= nil, "protocol closed")

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
    if not self.running then
        return
    end
    self.running = false
    for _, ctx in pairs(self.sockets) do
        destroySocket(ctx)
    end
    self.sockets = {}
    self.scanMqs = {}
    self.requestMqs = {}
    self.virtualDid = nil
end

---Create a miIO protocol runtime.
---@param netifs? string[] Network interface names.
---@param virtualDid? integer Virtual device ID: 64-bit.
---@return MiioProtocolRuntime runtime
---@nodiscard
function M.create(netifs, virtualDid)
    netifs, virtualDid = normalizeCreateArgs(netifs, virtualDid)
    local localPort, sockets = createSockets(netifs)

    ---@class MiioProtocolRuntime
    local o = {
        sockets = sockets,
        localPort = localPort,
        running = true,
        scanMqs = {},
        requestMqs = {},
        virtualDid = virtualDid or createVirtualDid(),
        _reqid = 0,
    }

    setmetatable(o, {
        __index = runtime
    })

    for _, ctx in pairs(o.sockets) do
        ctx.reader = core.createTimer(receiveLoop, o, ctx)
        ctx.reader:start(0)
    end

    return o
end

return M
