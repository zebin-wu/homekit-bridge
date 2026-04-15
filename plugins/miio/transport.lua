local socket = require "socket"
local netiflib = require "netif"

local assert = assert
local error = error
local type = type
local ipairs = ipairs
local pairs = pairs
local tostring = tostring
local xpcall = xpcall
local traceback = debug.traceback

local M = {}
local logger = log.getLogger("miio.transport")

local UDP_PORT = 54321
local MAX_MSG_LEN = 2048

---@class MiioTransportNetif
---@field key string
---@field addr string
---@field bindif netif|string

---@class MiioTransportSocket
---@field key string
---@field addr string
---@field bindif netif|string
---@field sock Socket
---@field reader Timer

---@class MiioTransport
---@field netifs MiioTransportNetif[]
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

local function normalizeNetif(netif)
    local netifType = type(netif)
    if netifType == "string" then
        return netiflib.find(netif)
    end
    if netifType == "userdata" then
        return netif
    end
    error("expected netif object or interface name")
end

local function resolveNetifs(netifs)
    if netifs ~= nil then
        assert(type(netifs) == "table", "netifs must be a table")
    end

    local results = {}
    local seen = {}
    local source = netifs or netiflib.getInterfaces()
    for i, entry in ipairs(source) do
        local netif = netifs and normalizeNetif(entry) or entry
        if isUsableNetif(netif) then
            local key = type(entry) == "string" and entry or netiflib.getIpv4Addr(netif)
            local addr = netiflib.getIpv4Addr(netif)
            if not seen[key] then
                table.insert(results, {
                    key = key,
                    addr = addr,
                    bindif = type(entry) == "string" and entry or netif,
                })
                seen[key] = true
            end
        elseif netifs ~= nil then
            error(("network interface #%d is not usable"):format(i))
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

    for _, entry in ipairs(netifs) do
        local sock = socket.create("UDP", "IPV4")
        local success, result = pcall(function()
            sock:settimeout(0)
            sock:enablebroadcast()
            sock:reuseaddr()
            sock:bindif(entry.bindif)
            sock:bind("0.0.0.0", localPort or 0)
            if localPort == nil then
                local _, port = sock:getsockname()
                localPort = port
            end
        end)
        if success then
            sockets[entry.key] = {
                key = entry.key,
                addr = entry.addr,
                bindif = entry.bindif,
                sock = sock,
            }
        else
            logger:error(("create socket error, %s, %s, %s"):format(entry.key, entry.addr, tostring(result)))
            pcall(sock.destroy, sock)
        end
    end

    if localPort ~= nil then
        return localPort, sockets
    end

    for _, ctx in pairs(sockets) do
        destroySocket(ctx)
    end
    error("failed to bind miio transport sockets")
end

local function receiveLoop(self, ctx)
    while self.running and self.sockets[ctx.key] == ctx do
        local success, data, addr, port = xpcall(ctx.sock.recvfrom, traceback, ctx.sock, MAX_MSG_LEN)
        if not success then
            if not self.running then
                return
            end
            logger:error(("recvfrom failed, %s, %s"):format(ctx.key, tostring(data)))
            return
        end
        if port == UDP_PORT then
            self:_dispatch(data, addr, port, ctx.key)
        end
    end
end

---@param self MiioTransport
function transport:_dispatch(data, addr, port, ifname)
    local subscribers = {}
    for id, handler in pairs(self.subscribers) do
        subscribers[#subscribers + 1] = {
            id = id,
            handler = handler,
        }
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
---@param netifs? (string|netif)[]
---@return MiioTransport transport
---@nodiscard
function M.create(netifs)
    local resolved = resolveNetifs(netifs)
    local localPort, sockets = createSockets(resolved)

    ---@type MiioTransport
    local o = {
        netifs = resolved,
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
        logger:info(("created socket, %s, %s, %s"):format(ctx.key, ctx.addr, localPort))
    end

    return o
end

return M
