local protocol = require "miio.protocol"
local timer = require "timer"
local ErrorCode = require "miio.error".code

local assert = assert
local type = type
local tunpack = table.unpack

local device = {}

---@alias DeviceState
---| '"NOINIT"'
---| '"INITED"'

---@class MiioDevice:MiioDevicePriv Device object.
local _device = {}

-- Declare function "dispatch".
local dispatch

local function _respCb(result, self, respCb, _, ...)
    self.requestable = true
    dispatch(self)
    respCb(self, result, ...)
end

local function _errCb(code, message, self, _, errCb, ...)
    self.logger:error(("Request error, code: %d, message: %s"):format(code, message))
    self.requestable  = true
    dispatch(self)
    errCb(self, code, message, ...)
end

---Dispatch the requests.
---@param self MiioDevice Device object.
dispatch = function (self)
    local que = self.cmdQue
    if que.first ~= que.last then
        local first = que.first
        local cmd = que[first]
        que[first] = nil
        que.first = first + 1

        self.pcb:request(_respCb, _errCb, self.timeout, tunpack(cmd))
        self.requestable  = false
    end
end

---Start a request and ``respCb`` will be called when a response is received.
---@param respCb fun(self: MiioDevice, result: any, ...) Response callback.
---@param errCb fun(self: MiioDevice, code: integer, message: string, ...) Error Callback.
---@param method string The request method.
---@param params? table Array of parameters.
function _device:request(respCb, errCb, method, params, ...)
    assert(type(respCb) == "function")
    assert(type(errCb) == "function")

    if self.requestable then
        self.pcb:request(_respCb, _errCb, self.timeout, method, params, self, respCb, errCb, ...)
        self.requestable  = false
        return
    end

    local que = self.cmdQue
    local last = que.last
    que[last] = {method, params, self, respCb, errCb, ...}
    que.last = last + 1
end

---Start sync properties.
function _device:startSync()
    self.logger:debug("Start sync properties.")
    local ms = math.random(3000, 6000)
    self.logger:debug(("Sync properties after %dms."):format(ms))
    self.timer:start(ms)
    self.isSyncing = true
end

---Stop sync properties.
function _device:stopSync()
    self.logger:debug("Stop sync properties.")
    self.timer:stop()
    self.isSyncing = false
end

---Register properties.
---@param names string[] Property names.
---@param update fun(self: MiioDevice, name: string, ...) The callback will be called when the property is updated.
function _device:registerProps(names, update, ...)
    assert(type(names) == "table")
    assert(type(update) == "function")

    self.update = update
    self.updateArgs = {...}
    self.isSyncing = false
    self.timer = timer.create(function (self, names)
        self.logger:debug("Syncing properties ...")
        self:request(function (self, result, names)
            if self.isSyncing == false then
                return
            end
            assert(#result == #names)
            local props = self.props
            for i, v in ipairs(result) do
                local name = names[i]
                if props[name] ~= v then
                    props[name] = v
                    self:update(name, tunpack(self.updateArgs))
                end
            end
            self:startSync()
        end, function (self, code, message)
            if self.isSyncing then
                self:startSync()
            end
        end, "get_prop", names, names)
    end, self, names)

    self:request(function (self, result, names)
        assert(#result == #names)
        for i, v in ipairs(result) do
            self.props[names[i]] = v
        end
        self:startSync()
    end, function (self, code, message)
        self:startSync()
    end, "get_prop", names, names)
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
    local props = self.props
    if props[name] == value then
        return
    end
    self:stopSync()
    props[name] = value

    self:request(function (self, result, name)
        self:update(name, tunpack(self.updateArgs))
        self:startSync()
    end, function (self, code, message)
        self:startSync()
    end, "set_" .. name, {value}, name)
end

---Start Handshake,  and the ``done`` callback will be called when the handshake is done.
---@param self MiioDevice Device object.
---@param done fun(self: MiioDevice, err: integer, ...) Done callback.
---@vararg any Arguments passed to the callback.
local function handshake(self, done, ...)
    local scanPriv = {
        self = self,
        done = done,
        args = {...}
    }

    scanPriv.timer = timer.create(function (priv)
        local self = priv.self
        self.logger:error("Scan timeout.")
        priv.ctx:stop()
        priv.ctx = nil
        priv.done(self, ErrorCode.Timeout, tunpack(priv.args))
    end, scanPriv)

    scanPriv.ctx = protocol.scan(function (addr, devid, stamp, priv)
        local self = priv.self
        priv.timer:stop()
        local pcb = protocol.create(addr, devid, self.token, stamp)
        if not pcb then
            self.logger:error("Failed to create PCB.")
            priv.done(self, ErrorCode.Unknown, tunpack(priv.args))
            return
        end
        self.pcb = pcb
        priv.done(self, ErrorCode.None, tunpack(priv.args))
    end, self.addr, scanPriv)
    if not scanPriv.ctx then
        self.logger:error("Failed to start scanning.")
        return nil
    end

    scanPriv.timer:start(self.timeout)
end

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

---Create a device object.
---@param done fun(self: MiioDevice, info: MiioDeviceInfo, ...) Callback will be called after the device is created.
---@param timeout integer Timeout period (in milliseconds).
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice obj Device object.
function device.create(done, timeout, addr, token, ...)
    assert(type(done) == "function")
    assert(timeout > 0, "timeout must be greater then 0")
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class MiioDevicePriv:table
    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        pcb = nil, ---@type MiioPcb
        addr = addr,
        token = token,
        timeout = timeout,
        requestable = true,
        cmdQue = { first = 0, last = 0 },
        props = {}
    }

    handshake(o, function (self, err, done, ...)
        if err ~= ErrorCode.None then
            done(self, nil, ...)
            return
        end
        self:request(function (self, result, done, ...)
            done(self, result, ...)
        end, function (self, code, message, done, ...)
            done(self, nil, ...)
        end, "miIO.info", nil, done, ...)
    end, done, ...)

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
