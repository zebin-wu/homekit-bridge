local socket = require "socket"
local netiflib = require "netif"

local assert = assert
local error = error
local type = type
local ipairs = ipairs
local pairs = pairs
local tostring = tostring
local xpcall = xpcall
local tconcat = table.concat
local traceback = debug.traceback

local M = {}
local logger = log.getLogger("miio.transport")

local UDP_PORT = 54321
local MAX_MSG_LEN = 2048

---@class MiioTransportSocket
---@field ifname string
---@field sock Socket
---@field reader Timer

---@class MiioTransport
---@field sockets table<string, MiioTransportSocket>
---@field localPort integer
---@field subscribers table<integer, fun(data:string, addr:string, port:integer, ifname:string)>
---@field running boolean
local transport = {}

local function isUsableNetif(netif)
    if not netiflib.isUp(netif) then
        return false
    end
    local addr = netiflib.getIpv4Addr(netif)
    return addr ~= "0.0.0.0" and addr ~= "127.0.0.1"
end

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

local function snapshotSubscribers(self)
    if next(self.subscribers) == nil then
        return nil
    end

    local subscribers = {}
    for id, handler in pairs(self.subscribers) do
        subscribers[#subscribers + 1] = {
            id = id,
            handler = handler,
        }
    end
    return subscribers
end

---@param self MiioTransport
function transport:_dispatch(data, addr, port, ifname)
    local subscribers = snapshotSubscribers(self)
    if subscribers == nil then
        return
    end

    for _, entry in ipairs(subscribers) do
        if self.subscribers[entry.id] ~= entry.handler then
            goto continue
        end
        local success, result = xpcall(entry.handler, traceback, data, addr, port, ifname)
        if not success then
            logger:error(("subscriber[%s] failed: %s"):format(entry.id, tostring(result)))
        end
::continue::
    end
end

local function receiveLoop(self, ctx)
    while self.running and self.sockets[ctx.ifname] == ctx do
        local ok, data, addr, port = receivePacket(ctx)
        if ok == nil then
            if not self.running then
                return
            end
            logger:error(("recvfrom failed, %s, %s"):format(ctx.ifname, tostring(data)))
            return
        end
        if ok then
            self:_dispatch(data, addr, port, ctx.ifname)
        end
    end
end

---@param self MiioTransport
function transport:close()
    if not self.running then
        return
    end
    self.running = false
    for _, ctx in pairs(self.sockets) do
        destroySocket(ctx)
    end
    self.sockets = {}
    self.subscribers = {}
end

---@param self MiioTransport
---@param data string
---@param addr string
---@param port integer
---@param ifname? string
---@return integer sent
function transport:sendto(data, addr, port, ifname)
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

---@param self MiioTransport
---@param handler fun(data:string, addr:string, port:integer, ifname:string)
---@return integer id
function transport:subscribe(handler)
    assert(type(handler) == "function")
    self._subId = (self._subId or 0) + 1
    self.subscribers[self._subId] = handler
    return self._subId
end

---@param self MiioTransport
---@param id integer
function transport:unsubscribe(id)
    self.subscribers[id] = nil
end

---Create a resident miIO UDP transport.
---@param netifs? string[]
---@return MiioTransport transport
---@nodiscard
function M.create(netifs)
    local resolved = resolveNetifs(netifs)
    local localPort, sockets = createSockets(resolved)

    ---@type MiioTransport
    local o = {
        sockets = sockets,
        localPort = localPort,
        subscribers = {},
        running = true,
        _subId = 0,
    }

    setmetatable(o, {
        __index = transport
    })

    for _, ctx in pairs(o.sockets) do
        ctx.reader = core.createTimer(receiveLoop, o, ctx)
        ctx.reader:start(0)
    end

    return o
end

return M
