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

-- Declare function "_dispatch".
local _dispatch

local function _respCb(result, self, respCb, _, ...)
    self.requestable = true
    _dispatch(self)
    respCb(self, result, ...)
end

local function _errCb(code, message, self, _, errCb, ...)
    self.logger:error(("Request error, code: %d, message: %s"):format(code, message))
    self.requestable  = true
    _dispatch(self)
    errCb(self, code, message, ...)
end

---Dispatch the requests.
---@param self MiioDevice Device object.
_dispatch = function (self)
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

---Start Handshake,  and the ``done`` callback will be called when the handshake is done.
---@param self MiioDevice Device object.
---@param done fun(self: MiioDevice, err: integer, ...) Done callback.
---@vararg any Arguments passed to the callback.
---@return boolean status true on success, false on failure.
local function _handshake(self, done, ...)
    local scanPriv = {
        self = self,
        done = done,
        args = {...}
    }

    scanPriv.timer = timer.create(function (priv)
        local self = priv.self
        self.logger:error("Handshake timeout.")
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
        self.logger:debug("Handshake done.")
        priv.done(self, ErrorCode.None, tunpack(priv.args))
    end, self.addr, scanPriv)
    if not scanPriv.ctx then
        return false
    end

    scanPriv.timer:start(self.timeout)
    self.logger:debug("Start handshake.")

    return true
end

---Recover the connection to the device.
---@param self MiioDevice Device object.
local function _recover(self)
    self.logger:debug("Recover connection ...")
    _handshake(self, function (self, err, ...)
        if err == ErrorCode.None then
            self.state = "INITED"
            self:startSync()
            return
        end
        _recover(self)
    end)
end

---Set property.
---@param self MiioDevice Device object.
---@param name string Property name.
---@param value any Property value.
---@param retry integer Retry count.
local function _setProp(self, name, value, retry)
    self:request(function (self, result, name)
        self:startSync()
        self:update(name, tunpack(self.updateArgs))
    end, function (self, code, message, name, value, retry)
        if code == ErrorCode.Timeout and retry > 0 then
            _setProp(self, name, value, retry - 1)
        else
            self.logger:error("Failed to set property.")
            if self.state ~= "NOINIT" then
                self.state = "NOINIT"
                _recover(self)
            end
        end
    end, "set_" .. name, {value}, name, value, retry)
end

---Start a request and ``respCb`` will be called when a response is received.
---@param respCb fun(self: MiioDevice, result: any, ...) Response callback.
---@param errCb fun(self: MiioDevice, code: integer, message: string, ...) Error Callback.
---@param method string The request method.
---@param params? table Array of parameters.
function _device:request(respCb, errCb, method, params, ...)
    assert(self.state == "INITED")
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
    assert(self.state == "INITED")

    self.logger:debug("Start sync properties.")
    local ms = math.random(3000, 6000)
    self.logger:debug(("Sync properties after %dms."):format(ms))
    self.timer:start(ms)
    self.isSyncing = true
end

---Stop sync properties.
function _device:stopSync()
    assert(self.state == "INITED")

    self.logger:debug("Stop sync properties.")
    self.timer:stop()
    self.isSyncing = false
end

---Register properties.
---@param names string[] Property names.
---@param update fun(self: MiioDevice, name: string, ...) The callback will be called when the property is updated.
function _device:registerProps(names, update, ...)
    assert(self.state == "INITED")
    assert(type(names) == "table")
    assert(type(update) == "function")

    self.update = update
    self.updateArgs = {...}
    self.isSyncing = false
    self.syncRetry = 3

    ---Sync property error callback.
    ---@param self MiioDevice Device object.
    ---@param code integer Error code.
    ---@param message string Error message.
    local function errCb(self, code, message)
        local retry = self.syncRetry
        if code == ErrorCode.Timeout and retry == 0 then
            self.logger:error("Failed to get properties.")
            if self.state ~= "NOINIT" then
                self.state = "NOINIT"
                _recover(self)
            end
            return
        else
            self.syncRetry = retry - 1
            if self.isSyncing then
                self:startSync()
            end
        end
    end

    self.timer = timer.create(function (self, names)
        self.logger:debug("Syncing properties ...")
        self:request(function (self, result, names)
            self.syncRetry = 3
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
        end, errCb, "get_prop", names, names)
    end, self, names)

    self:request(function (self, result, names)
        assert(#result == #names)
        for i, v in ipairs(result) do
            self.props[names[i]] = v
        end
        self:startSync()
    end, errCb, "get_prop", names, names)
end

---Get property.
---@param name string Property name.
---@return any value Property value.
function _device:getProp(name)
    assert(self.state == "INITED")

    return self.props[name]
end

---Set property.
---@param name string Property name.
---@param value any Property value.
function _device:setProp(name, value)
    assert(self.state == "INITED")

    local props = self.props
    if props[name] == value then
        return
    end
    props[name] = value
    self:stopSync()

    _setProp(self, name, value, 3)
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
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice obj Device object.
function device.create(done, addr, token, ...)
    assert(type(done) == "function")
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class MiioDevicePriv:table
    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        state = "NOINIT", ---@type DeviceState
        pcb = nil, ---@type MiioPcb
        addr = addr,
        token = token,
        timeout = 3000,
        requestable = true,
        cmdQue = { first = 0, last = 0 },
        props = {}
    }

    if _handshake(o, function (self, err, done, ...)
        if err ~= ErrorCode.None then
            done(self, nil, ...)
            return
        end
        self.state ="INITED"
        self:request(function (self, result, done, ...)
            done(self, result, ...)
        end, function (self, code, message, done, ...)
            done(self, nil, ...)
        end, "miIO.info", nil, done, ...)
    end, done, ...) == false then
        o.logger:error("Failed to start handshake.")
        return nil
    end

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
