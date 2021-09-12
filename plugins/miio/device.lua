local protocol = require "miio.protocol"
local timer = require "timer"

local device = {}

---@class MiioDevice:table Device object.
---
---@field props table Properties table.
---@field cmdQue table Command queue.
---@field logger logger Logger object.
---@field pcb MiioPcb Miio PCB.
---@field timeout integer Request timeout.
local _device = {}

---@alias DeviceState
---| '"NOINIT"'
---| '"IDLE"'
---| '"BUSY"'

---@class MiioDeviceNetIf:table Device network interface.
---
---@field gw string gateway IP.
---@field localIp string local IP.
---@field mask string mask of the network.

---@class MiioDeviceInfo:table Device information.
---
---@field model string Model, format: "{mfg}.{product}.{submodel}".
---@field mac string MAC address.
---@field fw_ver string Firmware version.
---@field hw_ver string Hardware version.
---@field netif MiioDeviceNetIf Network interface.

local function enque(que, ...)
    local last = que.last
    que[last] = {...}
    que.last = last + 1
end

local function deque(que)
    local first = que.first
    local cmd = que[first]
    que[first] = nil
    que.first = first + 1
    return table.unpack(cmd)
end

---Dispatch the requests.
---@param self MiioDevice
local function dispatch(self)
    if self.state ~= "IDLE" then
        return
    end
    local que = self.cmdQue
    if que.first ~= que.last then
        self.pcb:request(function (err, result, self, respCb, ...)
            respCb(err, result, ...)
            self.state = "IDLE"
            dispatch(self)
        end, self.timeout, deque(que))
        self.state = "BUSY"
    end
end

local function handleDoneCb(self, info)
    self.done(self, info, table.unpack(self.args))
    self.done = nil
    self.args = nil
end

---Handle sync proeprties.
---@param self MiioDevice
local function handleSyncProps(self)
    self.logger:debug("Syncing properties ...")
    self:request(function (err, result, self)
        local names = self.propNames
        if result then
            assert(#result == #names)
            local props = self.props
            for i, v in ipairs(result) do
                local name = names[i]
                if props[name] ~= v then
                    props[name] = v
                    self:update(name, table.unpack(self.updateArgs))
                end
            end
        end
        local ms = math.random(3000, 5000)
        self.logger:debug(("Sync properties after %dms."):format(ms))
        self.timer:start(ms, handleSyncProps, self)
    end, "get_prop", self.propNames, self)
end

---Start a request and ``respCb`` will be called when a response is received.
---@param respCb fun(err: MiioError|string|nil, result: any, ...) Response callback.
---@param method string The request method.
---@param params? table Array of parameters.
function _device:request(respCb, method, params, ...)
    assert(type(respCb) == "function")

    enque(self.cmdQue, method, params, self, respCb, ...)
    dispatch(self)
end

---Start syncing properties.
function _device:startSyncProps()
    self:request(function (err, result, self)
        local names = self.propNames
        if result then
            assert(#result == #names)
            for i, v in ipairs(result) do
                self.props[names[i]] = v
            end
        end
        local ms = math.random(3000, 6000)
        self.logger:debug(("Sync properties after %dms ..."):format(ms))
        self.timer:start(ms, handleSyncProps, self)
    end, "get_prop", self.propNames, self)
end

---Register properties.
---@param names string[] Property names.
---@param update fun(self: MiioDevice, name: string, ...) The callback will be called when the property is updated.
function _device:registerProps(names, update, ...)
    assert(type(names) == "table")
    assert(type(update) == "function")

    self.update = update
    self.propNames = names
    self.updateArgs = {...}
    self.timer = timer.create()

    self:startSyncProps()
end

---Get property.
---@param name string Property name.
---@return any value Property value.
function _device:getProp(name)
    return self.props[name]
end

---Set property.
---@param name string Property name.
---@param value any Property value.
function _device:setProp(name, value)
    if self.props[name] == value then
        return
    end
    self.props[name] = value
    self:request(function (err, result, self, name)
        if result then
            self:update(name, table.unpack(self.updateArgs))
        end
    end, "set_" .. name, {value}, self, name)
end

---Create a device object.
---@param done fun(self: MiioDevice, info: MiioDeviceInfo, ...) Callback will be called after the device is created.
---@param timeout integer Timeout period (in milliseconds).
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice|nil obj Device object.
function device.create(done, timeout, addr, token, ...)
    assert(type(done) == "function")
    assert(timeout > 0, "timeout must be greater then 0")
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        done = done,
        args = {...},
        timeout = timeout,
        state = "NOINIT",
        cmdQue = {first = 0, last = 0},
        props = {}
    }

    o.scanCtx = protocol.scan(function (addr, devid, stamp, self, token)
        self.scanTimer:cancel()
        self.scanTimer = nil
        self.scanCtx = nil
        local pcb = protocol.create(addr, devid, token, stamp)
        if not pcb then
            self.logger:error("Failed to create PCB.")
            handleDoneCb(self)
            return
        end
        self.pcb = pcb
        self.state = "IDLE"
        self:request(function (err, result, self)
            handleDoneCb(self, result)
        end, "miIO.info", nil, self)
    end, addr, o, token)
    if not o.scanCtx then
        o.logger:error("Failed to start scanning.")
        return nil
    end

    o.scanTimer = timer.create()
    o.scanTimer:start(timeout, function (self)
        self.logger:error("Scan timeout.")
        self.scanCtx:stop()
        self.scanCtx = nil
        self.scanTimer = nil
        handleDoneCb(self)
    end, o)

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
