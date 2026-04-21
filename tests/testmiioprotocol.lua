local core = require "core"
local socket = require "socket"
local netif = require "netif"
local hash = require "hash"
local cipher = require "cipher"
local json = require "cjson"
local protocol = require "miio.protocol"

local floor = math.floor
local ipairs = ipairs
local spack = string.pack
local sunpack = string.unpack
local srep = string.rep
local schar = string.char
local tconcat = table.concat

local UDP_PORT = 54321
local MAX_MSG_LEN = 2048

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

local function createEncryption(token)
    local ctx = cipher.create("AES-128-CBC")
    ctx:setPadding("PKCS7")

    local key = md5(token)
    local iv = md5(key, token)

    return {
        encrypt = function(_, input)
            return ctx:process("encrypt", key, iv, input)
        end,
        decrypt = function(_, input)
            return ctx:process("decrypt", key, iv, input)
        end,
    }
end

local function pack(did, stamp, token, data)
    local len = 32 + (data and #data or 0)
    local header = spack(">I2>I2>I8>I4", 0x2131, len, did, stamp)
    local checksum = token and md5(header, token, data or "") or srep(schar(0xff), 16)
    return tconcat({header, checksum, data or ""})
end

local function unpack(packet, token)
    assert(sunpack(">I2", packet, 1) == 0x2131)
    local len = sunpack(">I2", packet, 3)
    assert(len == #packet and len >= 32)

    local data = nil
    if len > 32 then
        data = sunpack("c" .. len - 32, packet, 33)
    end

    if token then
        assert(md5(sunpack("c16", packet, 1), token, data or "") == sunpack("c16", packet, 17))
    end

    return sunpack(">I8", packet, 5),
        sunpack(">I4", packet, 13),
        data
end

local function findTestNetif()
    for _, iface in ipairs(netif.getInterfaces()) do
        if netif.isUp(iface) then
            local addr = netif.getIpv4Addr(iface)
            if addr ~= "0.0.0.0" and addr ~= "127.0.0.1" then
                return netif.getName(iface), addr
            end
        end
    end
    error("no usable non-loopback ipv4 interface")
end

local function waitFor(mq, timeout)
    local success, ok, result = mq:recvUntil(floor(core.time()) + timeout)
    if not success then
        return false, ok
    end
    return ok, result
end

local function waitUntil(timeout, predicate)
    local deadline = floor(core.time()) + timeout
    while floor(core.time()) < deadline do
        local result = predicate()
        if result then
            return result
        end
        core.sleep(5)
    end
    return predicate()
end

local function startUdpDevice(handler)
    local server = socket.create("UDP", "IPV4")
    server:reuseaddr()
    server:bind("0.0.0.0", UDP_PORT)

    core.createTimer(function ()
        while true do
            local msg, addr, port = server:recvfrom(MAX_MSG_LEN)
            if #msg == 0 then
                server:destroy()
                return
            end
            handler(msg, addr, port, server)
        end
    end):start(0)

    return function(targetAddr)
        local wake <close> = socket.create("UDP", "IPV4")
        wake:sendto("", targetAddr, UDP_PORT)
        core.sleep(20)
    end
end

-- Tests wildcard scan waiter is registered while waiting and removed after timeout.
do
    local bindif = findTestNetif()
    local runtime = protocol.create({bindif}, 0x1111222233334444)
    local mq = core.createMQ(1)

    core.createTimer(function()
        local ok, results = pcall(runtime.scan, runtime, 50)
        mq:send(ok, results)
    end):start(0)

    assert(waitUntil(100, function()
        local waiters = runtime.scanMqs["*"]
        return waiters ~= nil and #waiters == 1
    end))

    local ok, results = waitFor(mq, 500)
    assert(ok == true)
    assert(type(results) == "table")
    assert(runtime.scanMqs["*"] == nil)

    runtime:close()
end

-- Tests protocol.scan/request over the resident protocol runtime with a virtual miIO device.
do
    local bindif, addr = findTestNetif()
    local token = "0123456789abcdef"
    local virtualDid = 0x1111222233334444
    local deviceDid = 0x0102030405060708
    local deviceStamp = floor(core.time()) - 5
    local enc = createEncryption(token)
    local stats = {
        probes = 0,
        requests = 0,
    }

    local stopDevice = startUdpDevice(function(msg, fromAddr, fromPort, server)
        local ok, did, stamp, data = pcall(unpack, msg)
        if ok and did == -1 and stamp == 0xffffffff and data == nil then
            stats.probes = stats.probes + 1
            local resp = pack(deviceDid, deviceStamp)
            assert(server:sendto(resp, fromAddr, fromPort) == #resp)
            return
        end

        did, stamp, data = unpack(msg, token)
        assert(did == deviceDid)
        local req = json.decode(enc:decrypt(data))
        stats.requests = stats.requests + 1

        local payload = json.encode({
            id = req.id,
            result = {
                req.method,
                req.params and req.params[1] or false,
                stats.requests,
            },
        })
        local resp = pack(deviceDid, deviceStamp, token, enc:encrypt(payload))
        assert(server:sendto(resp, fromAddr, fromPort) == #resp)
    end)

    local runtime = protocol.create({bindif}, virtualDid)

    local results = runtime:scan(1000, addr)
    assert(#results == 1)
    assert(results[1].addr == addr)
    assert(results[1].devid == deviceDid)
    assert(results[1].ifname == bindif)
    assert(runtime.scanMqs[addr] == nil)
    assert(runtime.scanMqs["*"] == nil)

    local pcb = runtime:createPcb(addr, token)
    local result1 = pcb:request(1000, "test.echo", "ping")
    assert(result1[1] == "test.echo")
    assert(result1[2] == "ping")
    assert(result1[3] == 1)
    assert(pcb.ifname == bindif)
    assert(runtime.requestMqs[addr] == nil)

    local result2 = pcb:request(1000, "test.echo", "pong")
    assert(result2[1] == "test.echo")
    assert(result2[2] == "pong")
    assert(result2[3] == 2)
    assert(runtime.requestMqs[addr] == nil)

    assert(stats.probes == 2)
    assert(stats.requests == 2)

    stopDevice(addr)
    runtime:close()
end

-- Tests concurrent PCBs talking to the same device keep request/response pairs isolated.
do
    local bindif, addr = findTestNetif()
    local token = "0123456789abcdef"
    local deviceDid = 0x1112131415161718
    local deviceStamp = floor(core.time()) - 5
    local enc = createEncryption(token)
    local pending = {}
    local readyMq = core.createMQ(1)
    local releaseMq = core.createMQ(1)

    local stopDevice = startUdpDevice(function(msg, fromAddr, fromPort, server)
        local ok, did, stamp, data = pcall(unpack, msg)
        if ok and did == -1 and stamp == 0xffffffff and data == nil then
            local resp = pack(deviceDid, deviceStamp)
            assert(server:sendto(resp, fromAddr, fromPort) == #resp)
            return
        end

        did, stamp, data = unpack(msg, token)
        assert(did == deviceDid)

        local req = json.decode(enc:decrypt(data))
        pending[#pending + 1] = {
            addr = fromAddr,
            port = fromPort,
            req = req,
        }
        if #pending < 2 then
            return
        end

        readyMq:send(true)
        releaseMq:recv()
        for i = 1, #pending do
            local item = pending[i]
            local payload = json.encode({
                id = item.req.id,
                result = {
                    item.req.method,
                    item.req.params[1],
                },
            })
            local resp = pack(deviceDid, deviceStamp, token, enc:encrypt(payload))
            assert(server:sendto(resp, item.addr, item.port) == #resp)
        end
        pending = {}
    end)

    local runtime = protocol.create({bindif}, 0x5555666677778888)
    local pcb1 = runtime:createPcb(addr, token)
    local pcb2 = runtime:createPcb(addr, token)
    local mq = core.createMQ(2)

    core.createTimer(function()
        local ok, result = pcall(pcb1.request, pcb1, 1000, "test.echo", "A")
        mq:send("pcb1", ok, result)
    end):start(0)

    core.createTimer(function()
        local ok, result = pcall(pcb2.request, pcb2, 1000, "test.echo", "B")
        mq:send("pcb2", ok, result)
    end):start(0)

    local ready = waitFor(readyMq, 1000)
    assert(ready == true)
    assert(waitUntil(100, function()
        local waiters = runtime.requestMqs[addr]
        return waiters ~= nil and #waiters == 2
    end))

    releaseMq:send(true)

    local results = {}
    for _ = 1, 2 do
        local name, ok, result = mq:recv()
        assert(ok == true, result)
        results[name] = result
    end

    assert(results.pcb1[1] == "test.echo")
    assert(results.pcb1[2] == "A")
    assert(results.pcb2[1] == "test.echo")
    assert(results.pcb2[2] == "B")
    assert(runtime.requestMqs[addr] == nil)

    stopDevice(addr)
    runtime:close()
end

-- Tests timed out requests remove their waiter entry from the runtime.
do
    local bindif, addr = findTestNetif()
    local token = "0123456789abcdef"
    local deviceDid = 0x2122232425262728
    local deviceStamp = floor(core.time()) - 5
    local requestSeen = core.createMQ(1)

    local stopDevice = startUdpDevice(function(msg, fromAddr, fromPort, server)
        local ok, did, stamp, data = pcall(unpack, msg)
        if ok and did == -1 and stamp == 0xffffffff and data == nil then
            local resp = pack(deviceDid, deviceStamp)
            assert(server:sendto(resp, fromAddr, fromPort) == #resp)
            return
        end

        did, stamp, data = unpack(msg, token)
        assert(did == deviceDid)
        requestSeen:send(true, fromAddr, fromPort)
    end)

    local runtime = protocol.create({bindif}, 0x9999000011112222)
    local pcb = runtime:createPcb(addr, token)
    pcb:handshake(1000)

    local mq = core.createMQ(1)
    core.createTimer(function()
        local ok, err = pcall(pcb.request, pcb, 60, "test.timeout")
        mq:send(ok, err)
    end):start(0)

    local ok = waitFor(requestSeen, 1000)
    assert(ok == true)
    assert(waitUntil(100, function()
        local waiters = runtime.requestMqs[addr]
        return waiters ~= nil and #waiters == 1
    end))

    local success, err = waitFor(mq, 500)
    assert(success == false)
    assert(tostring(err):find("timeout", 1, true) ~= nil)
    assert(runtime.requestMqs[addr] == nil)
    assert(pcb.ifname == nil)
    assert(pcb.stampDiff == nil)

    stopDevice(addr)
    runtime:close()
end
