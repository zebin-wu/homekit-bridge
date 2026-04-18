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
    for ifname in pairs(tp.sockets) do
        count = count + 1
        assert(netif.getIpv4Addr(netif.find(ifname)) ~= "127.0.0.1")
    end
    assert(count > 0)
    tp:close()
end

-- Tests transport creation fails instead of silently dropping requested interfaces.
do
    local origSocketCreate = socket.create
    local origGetInterfaces = netif.getInterfaces
    local origGetName = netif.getName
    local origIsUp = netif.isUp
    local origGetIpv4Addr = netif.getIpv4Addr
    local origFind = netif.find

    local destroyed = {}
    local fakeIfs = {
        eth0 = {
            name = "eth0",
            addr = "192.168.10.2",
        },
        wlan0 = {
            name = "wlan0",
            addr = "192.168.20.2",
        },
    }
    local nextPort = 40000

    local function restore()
        socket.create = origSocketCreate
        netif.getInterfaces = origGetInterfaces
        netif.getName = origGetName
        netif.isUp = origIsUp
        netif.getIpv4Addr = origGetIpv4Addr
        netif.find = origFind
    end

    local ok, err = pcall(function()
        netif.getInterfaces = function()
            return {fakeIfs.eth0, fakeIfs.wlan0}
        end
        netif.getName = function(iface)
            return iface.name
        end
        netif.isUp = function(_)
            return true
        end
        netif.getIpv4Addr = function(iface)
            return iface.addr
        end
        netif.find = function(name)
            return assert(fakeIfs[name])
        end

        socket.create = function(sockType, familyName)
            assert(sockType == "UDP")
            assert(familyName == "IPV4")
            nextPort = nextPort + 1

            local o = {
                port = nextPort,
            }

            function o:settimeout(ms)
                assert(ms == 0)
            end

            function o:enablebroadcast() end

            function o:reuseaddr() end

            function o:bindif(ifname)
                self.ifname = ifname
                if ifname == "wlan0" then
                    error("bindif failed")
                end
            end

            function o:bind(addr, port)
                assert(addr == "0.0.0.0")
                assert(type(port) == "number")
            end

            function o:getsockname()
                return "0.0.0.0", self.port
            end

            function o:destroy()
                destroyed[self.ifname] = true
            end

            return o
        end

        local created, createErr = pcall(transport.create, {"eth0", "wlan0"})
        assert(created == false)
        assert(createErr:find("failed to bind miio transport sockets", 1, true) ~= nil)
        assert(createErr:find("wlan0", 1, true) ~= nil)
    end)

    restore()
    assert(ok, err)
    assert(destroyed.eth0 == true)
    assert(destroyed.wlan0 == true)
end

-- Tests resident transport send/recv flow, source-port filtering and unsubscribe.
do
    local bindif, addr = findTestNetif()
    local tp = transport.create({bindif})
    local ctx = assert(tp.sockets[bindif])
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

    assert(tp:sendto("ping", addr, UDP_PORT, bindif) == 1)
    local ok, packet = waitFor(recvMq, 1000)
    assert(ok == true)
    assert(packet.data == "pong")
    assert(packet.addr == addr)
    assert(packet.port == UDP_PORT)
    assert(packet.ifname == bindif)

    local other <close> = socket.create("UDP", "IPV4")
    assert(other:sendto("ignored", addr, tp.localPort) == 7)
    ok = waitFor(recvMq, 100)
    assert(ok == false)

    tp:unsubscribe(subId)
    assert(tp:sendto("notify", addr, UDP_PORT, bindif) == 1)
    ok = waitFor(recvMq, 100)
    assert(ok == false)

    stopDevice(addr)
    tp:close()
    tp:close()
end

-- Tests protocol.scan/request over the resident transport with a virtual miIO device.
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

    local pcb = runtime:createPcb(addr, token)
    local result1 = pcb:request(1000, "test.echo", "ping")
    assert(result1[1] == "test.echo")
    assert(result1[2] == "ping")
    assert(result1[3] == 1)
    assert(pcb.ifname == bindif)

    local result2 = pcb:request(1000, "test.echo", "pong")
    assert(result2[1] == "test.echo")
    assert(result2[2] == "pong")
    assert(result2[3] == 2)

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

        for _, item in ipairs(pending) do
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

    stopDevice(addr)
    runtime:close()
end
