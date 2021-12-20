local protocol = require "miio.protocol"
local time = require "time"

local assert = assert
local type = type
local tunpack = table.unpack
local tinsert = table.insert

local device = {}

---@class MiotIID: table MIOT instance ID.
---
---@field siid integer Service instance ID.
---@field piid integer Property instance ID.

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

---@alias DeviceState
---| '"NOINIT"'
---| '"INITED"'

---@class MiioDevice:MiioDevicePriv Device object.
local _device = {}

-- Declare function "_dispatch".
local _dispatch

local function _respCb(result, obj, respCb, _, ...)
    obj.requestable = true
    _dispatch(obj)
    respCb(obj, result, ...)
end

local function _errCb(code, message, obj, _, errCb, ...)
    obj.logger:error("Request error, code: " .. code .. ", message:  " .. message)
    obj.requestable  = true
    _dispatch(obj)
    errCb(obj, code, message, ...)
end

---Dispatch the requests.
---@param obj MiioDevice
_dispatch = function (obj)
    local que = obj.cmdQue
    if que.first ~= que.last then
        local first = que.first
        local cmd = que[first]
        que[first] = nil
        que.first = first + 1

        obj.pcb:request(_respCb, _errCb, obj.timeout, tunpack(cmd))
        obj.requestable  = false
    end
end

---Start Handshake,  and the ``done`` callback will be called when the handshake is done.
---@param obj MiioDevice Device object.
---@param done fun(obj: MiioDevice, err: '"None"'|'"Timeout"'|'"Unknown"', ...) Done callback.
---@vararg any Arguments passed to the callback.
local function handshake(obj, done, ...)
    ---@class MiioScanPriv
    local priv = {
        done = done,
        args = {...}
    }

    priv.timer = time.createTimer(
    ---@param obj MiioDevice
    ---@param priv MiioScanPriv
    function (obj, priv)
        obj.logger:error("Handshake timeout.")
        priv.ctx:stop()
        priv.done(obj, "Timeout", tunpack(priv.args))
    end, obj, priv)

    priv.ctx = protocol.scan(
    ---@param obj MiioDevice
    ---@param priv MiioScanPriv
    function (addr, devid, stamp, obj, priv)
        priv.timer:stop()
        local pcb = protocol.create(addr, devid, obj.token, stamp)
        if not pcb then
            obj.logger:error("Failed to create PCB.")
            priv.done(obj, "Unknown", tunpack(priv.args))
            return
        end
        obj.pcb = pcb
        obj.logger:debug("Handshake done.")
        priv.done(obj, "None", tunpack(priv.args))
    end, obj.addr, obj, priv)

    priv.timer:start(obj.timeout)
    obj.logger:debug("Start handshake.")
end

---Start sync properties after ``timeout`` ms.
---@param obj MiioDevice Device object.
---@param timeout? integer Timeout period (in milliseconds).
local function startSync(obj, timeout)
    assert(obj.state == "INITED")

    timeout = timeout or math.random(5000, 10000)
    obj.logger:debug(("Sync properties after %dms."):format(timeout))
    local pm = obj.pm
    pm.timer:start(timeout)
    pm.isSyncing = true
end

---Stop sync properties.
---@param obj MiioDevice Device object.
local function stopSync(obj)
    assert(obj.state == "INITED")

    obj.logger:debug("Stop sync properties.")
    local pm = obj.pm
    pm.timer:stop()
    pm.isSyncing = false
end

---Recover the connection to the device.
---@param obj MiioDevice Device object.
local function recover(obj)
    obj.logger:debug("Recover connection ...")
    local success = pcall(handshake, obj, function (obj, err)
        if err == "None" then
            obj.state = "INITED"
            startSync(obj)
            return
        end
        recover(obj)
    end)
    if success == false then
        time.createTimer(function (obj)
            recover(obj)
        end, obj):start(obj.timeout)
    end
end

---Start a request and ``respCb`` will be called when a response is received.
---@param respCb fun(obj: MiioDevice, result: any, ...) Response callback.
---@param errCb fun(obj: MiioDevice, code: integer|'"Timeout"'|'"Unknown"', message: string, ...) Error Callback.
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

---Create a property manager.
---@param syncTimer Timer Sync timer.
---@param onUpdate fun(self: MiioDevice, names: string[], ...) The callback will be called when the property is updated.
---@vararg any Arguments passed to the callback.
---@return MiioPropMgr
local function createPropMgr(syncTimer, onUpdate, ...)
    ---@class MiioPropMgr
    local pm = {
        timer = syncTimer,
        values = {}, ---@type table<string, string|number|boolean>
        onUpdate = onUpdate,
        args = {...},
        isSyncing = false,
        retry = 3
    }

    return pm
end

---Get properties error callback.
---@param obj MiioDevice Device object.
---@param code integer|'"Timeout"'|'"Unknown"' Error code.
---@param message string Error message.
local function getPropsErrCb(obj, code, message)
    if obj.state == "NOINIT" then
        return
    end
    local pm = obj.pm
    local retry = pm.retry
    if code == "Timeout" and retry == 0 then
        obj.logger:error("Failed to get properties.")
        obj.state = "NOINIT"
        recover(obj)
    else
        pm.retry = retry - 1
        if pm.isSyncing then
            startSync(obj)
        end
    end
end

---Register properties.
---@param names string[] Property names.
---@param onUpdate fun(obj: MiioDevice, names: string[], ...) The callback will be called when the property is updated.
function _device:regProps(names, onUpdate, ...)
    assert(self.state == "INITED")
    assert(type(names) == "table")
    assert(type(onUpdate) == "function")

    ---Sync properties.
    ---@param obj MiioDevice Device Object.
    ---@param names string[] Property names.
    local function syncProps(obj, names)
        obj.logger:debug("Syncing properties ...")
        obj:request(
        ---@param obj MiioDevice
        ---@param result any
        ---@param names string[]
        function (obj, result, names)
            if obj.state == "NOINIT" then
                return
            end
            local pm = obj.pm
            if pm.isSyncing == false then
                return
            end
            pm.retry = 3
            assert(#result == #names)
            local values = pm.values
            local updatedNames = {}
            for i, v in ipairs(result) do
                local name = names[i]
                if values[name] ~= v then
                    values[name] = v
                    tinsert(updatedNames, name)
                end
            end
            startSync(obj)
            if #updatedNames ~= 0 then
                pm.onUpdate(obj, updatedNames, tunpack(pm.args))
            end
        end, getPropsErrCb, "get_prop", names, names)
    end

    local pm = createPropMgr(time.createTimer(syncProps, self, names), onUpdate, ...)
    pm.isSyncing = true
    self.pm = pm

    syncProps(self, names)
end

---Register properties for MIOT protocol device.
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
---@param onUpdate fun(self: MiioDevice, names: string[], ...) The callback will be called when the property is updated.
function _device:regPropsMiot(mapping, onUpdate, ...)
    assert(self.state == "INITED")
    assert(type(mapping) == "table")
    assert(type(onUpdate) == "function")

    local params = {}
    for n, v in pairs(mapping) do
        tinsert(params, {
            did = n,
            siid = v.siid,
            piid = v.piid
        })
    end

    ---Sync properties for MIOT protocol device.
    ---@param obj MiioDevice
    ---@param params table
    local function syncPropsMiot(obj, params)
        obj.logger:debug("Syncing properties ...")
        obj:request(
        ---@param obj MiioDevice
        ---@param result any
        function (obj, result)
            if obj.state == "NOINIT" then
                return
            end
            local pm = obj.pm
            if pm.isSyncing == false then
                return
            end
            pm.retry = 3
            local values = pm.values
            local updatedNames = {}
            for _, v in ipairs(result) do
                local name = v.did
                if values[name] ~= v.value then
                    values[name] = v.value
                    tinsert(updatedNames, name)
                end
            end
            startSync(obj)
            if #updatedNames ~= 0 then
                pm.onUpdate(obj, updatedNames, tunpack(pm.args))
            end
        end, getPropsErrCb, "get_properties", params)
    end

    local pm = createPropMgr(time.createTimer(syncPropsMiot, self, params), onUpdate, ...)
    pm.mapping = mapping
    pm.isSyncing = true
    self.pm = pm

    syncPropsMiot(self, params)
end

---Get property.
---@param name string Property name.
---@return string|number|boolean value Property value.
function _device:getProp(name)
    assert(self.state == "INITED")

    return self.pm.values[name]
end

---Set property.
---@param obj MiioDevice Device object.
---@param name string Property name.
---@param method string The request method.
---@param params table Array of parameters.
---@param retry integer Retry count.
local function _setProp(obj, name, method, params, retry)
    obj:request(
    ---@param obj MiioDevice
    ---@param result any
    ---@param name string
    function (obj, result, name)
        if obj.state == "NOINIT" then
            return
        end
        local pm = obj.pm
        startSync(obj)
        pm.onUpdate(obj, {name}, tunpack(pm.args))
    end,
    ---@param obj MiioDevice
    ---@param code integer|'"Timeout"'|'"Unknown"'
    ---@param message string
    ---@param name string
    ---@param method string
    ---@param params table
    ---@param retry integer
    function (obj, code, message, name, method, params, retry)
        if obj.state == "NOINIT" then
            return
        end
        if code == "Timeout" and retry > 0 then
            _setProp(obj, name, method, params, retry - 1)
        else
            obj.logger:error("Failed to set property.")
            obj.state = "NOINIT"
            recover(obj)
        end
    end, method, params, name, method, params, retry)
end

---Set property.
---@param name string Property name.
---@param value string|number|boolean Property value.
function _device:setProp(name, value)
    assert(self.state == "INITED")

    local pm = self.pm
    local values = pm.values
    if values[name] == value then
        return
    end
    values[name] = value
    stopSync(self)
    local method
    local param
    if pm.mapping then
        method = "set_properties"
        param = {
            did = name,
            siid = pm.mapping[name].siid,
            piid = pm.mapping[name].piid,
            value = value
        }
    else
        method = "set_" .. name
        param = value
    end
    _setProp(self, name, method, { param }, pm.retry)
end

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

    ---@class MiioDevicePriv
    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        state = "NOINIT", ---@type DeviceState
        pcb = nil, ---@type MiioPcb
        addr = addr,
        token = token,
        timeout = 5000,
        requestable = true,
        cmdQue = { first = 0, last = 0 }
    }

    handshake(o,
    ---@param obj MiioDevice
    ---@param err '"None"'|'"Timeout"'|'"Unknown"'
    ---@param done fun(self: MiioDevice, info: MiioDeviceInfo, ...)
    function (obj, err, done, ...)
        if err ~= "None" then
            done(obj, nil, ...)
            return
        end
        obj.state ="INITED"
        obj:request(
        ---@param obj MiioDevice
        ---@param result any
        ---@param done fun(self: MiioDevice, info: MiioDeviceInfo, ...)
        function (obj, result, done, ...)
            done(obj, result, ...)
        end,
        ---@param obj MiioDevice
        ---@param code integer|'"Timeout"'|'"Unknown"'
        ---@param message string
        ---@param done fun(self: MiioDevice, info: MiioDeviceInfo, ...)
        function (obj, code, message, done, ...)
            done(obj, nil, ...)
        end, "miIO.info", nil, done, ...)
    end, done, ...)

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
