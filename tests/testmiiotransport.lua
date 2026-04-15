local core = require "core"
local socket = require "socket"
local netif = require "netif"
local hash = require "hash"
local cipher = require "cipher"
local json = require "cjson"
local transport = require "miio.transport"
local protocol = require "miio.protocol"

local floor = math.floor
local ipairs = ipairs
local pairs = pairs
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
                return iface, addr
            end
        end
    end
    error("no usable non-loopback ipv4 interface")
end

local function waitFor(mq, timeout)
    local timer = core.createTimer(function(q)
        q:send(false, "timeout")
    end, mq)

    timer:start(timeout)
    local ok, result = mq:recv()
    timer:stop()
    return ok, result
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

-- Tests auto-selected transport skips loopback interfaces.
do
    local tp = transport.create()
    local count = 0
    for _, entry in ipairs(tp.netifs) do
        count = count + 1
        assert(entry.addr ~= "127.0.0.1")
    end
    assert(count > 0)
    tp:close()
end

-- Tests resident transport send/recv flow, source-port filtering and unsubscribe.
do
    local iface, addr = findTestNetif()
    local tp = transport.create({iface})
    local ifname = tp.netifs[1].key
    local ctx = tp.sockets[ifname]
    local _, localPort = ctx.sock:getsockname()
    assert(localPort == tp.localPort)

    local recvMq = core.createMQ(4)
    local stopDevice = startUdpDevice(function(msg, fromAddr, fromPort, server)
        assert(fromAddr == addr)
        assert(fromPort == tp.localPort)

        if msg == "ping" then
            assert(server:sendto("pong", fromAddr, fromPort) == 4)
        elseif msg == "notify" then
            assert(server:sendto("after-unsub", fromAddr, fromPort) == 11)
        end
    end)

    local subId = tp:subscribe(function(data, fromAddr, port, inboundIfname)
        recvMq:send(true, {
            data = data,
            addr = fromAddr,
            port = port,
            ifname = inboundIfname,
        })
    end)

    assert(tp:sendto("ping", addr, UDP_PORT, ifname) == 1)
    local ok, packet = waitFor(recvMq, 1000)
    assert(ok == true)
    assert(packet.data == "pong")
    assert(packet.addr == addr)
    assert(packet.port == UDP_PORT)
    assert(packet.ifname == ifname)

    local other <close> = socket.create("UDP", "IPV4")
    assert(other:sendto("ignored", addr, tp.localPort) == 7)
    ok = waitFor(recvMq, 100)
    assert(ok == false)

    tp:unsubscribe(subId)
    assert(tp:sendto("notify", addr, UDP_PORT, ifname) == 1)
    ok = waitFor(recvMq, 100)
    assert(ok == false)

    stopDevice(addr)
    tp:close()
    tp:close()
end

-- Tests protocol.scan/request over the resident transport with a virtual miIO device.
do
    local iface, addr = findTestNetif()
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

    protocol.init({iface}, virtualDid)

    local results = protocol.scan(1000, addr)
    assert(#results == 1)
    assert(results[1].addr == addr)
    assert(results[1].devid == deviceDid)
    assert(results[1].ifname == addr)

    local pcb = protocol.create(addr, token)
    local result1 = pcb:request(1000, "test.echo", "ping")
    assert(result1[1] == "test.echo")
    assert(result1[2] == "ping")
    assert(result1[3] == 1)
    assert(pcb.ifname == addr)

    local result2 = pcb:request(1000, "test.echo", "pong")
    assert(result2[1] == "test.echo")
    assert(result2[2] == "pong")
    assert(result2[3] == 2)

    assert(stats.probes == 2)
    assert(stats.requests == 2)

    protocol.init({iface}, virtualDid + 1)
    stopDevice(addr)
end
