local protocol = require "miio.protocol"
local util = require "util"
local xpcall = xpcall
local traceback = debug.traceback
local assert = assert
local type = type
local tunpack = table.unpack
local tinsert = table.insert
local pairs = pairs
local tostring = tostring
local createFuture = core.createFuture
local isYieldable = core.isYieldable

local M = {}
local NULL = {}

---@class MiotIID:table MIOT instance ID.
---
---@field siid integer Service instance ID.
---@field piid integer Property instance ID.

---@class MiotProperty:table MIOT property.
---
---@field did any Device ID(Use this field to map aliases).
---@field siid integer Service instance ID.
---@field piid integer Property instance ID.
---@field code integer Error code.
---@field value any Proeprty value.

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

---@class MiioDevice Device object.
local device = {}

local function formatError(err)
    if type(err) == "table" then
        if err.message ~= nil and err.code ~= nil then
            return ("%s (code=%s)"):format(tostring(err.message), tostring(err.code))
        end
        if err.message ~= nil then
            return tostring(err.message)
        end
    end
    return tostring(err)
end

local function encodeValue(value)
    if value == nil then
        return NULL
    end
    return value
end

local function decodeValue(value)
    if value == NULL then
        return nil
    end
    return value
end

---@param names string[]
---@return string[]
local function normalizeNames(names)
    local unique = {}
    local seen = {}
    for _, name in ipairs(names) do
        assert(type(name) == "string")
        if seen[name] ~= true then
            seen[name] = true
            tinsert(unique, name)
        end
    end
    return unique
end

---Get properties.
---@param obj MiioDevice
---@param names string[]
---@return boolean success
---@return table<string, any>|string propsOrErr
---@return table<string, string>? errs
local function requestProps(obj, names)
    local success, result = xpcall(obj.request, traceback, obj, "get_prop", tunpack(names))
    if success == false then
        return success, formatError(result)
    end
    local props = {}
    local errs = {}
    for i, name in ipairs(names) do
        local value = result[i]
        if value == nil then
            errs[name] = ("missing property '%s' in response"):format(name)
        else
            props[name] = encodeValue(value)
        end
    end
    return success, props, errs
end

---Get properties(MIOT).
---@param obj MiioDevice
---@param names string[]
---@return boolean success
---@return table<string, any>|string propsOrErr
---@return table<string, string>? errs
local function requestPropsMiot(obj, names)
    local mapping = obj.mapping
    assert(mapping ~= nil, "missing mapping")
    local params = {}
    for _, name in ipairs(names) do
        local iid = assert(mapping[name], ("missing mapping for property '%s'"):format(name))
        tinsert(params, {
            did = name,
            siid = iid.siid,
            piid = iid.piid,
        })
    end
    local success, result = xpcall(obj.request, traceback, obj, "get_properties", tunpack(params))
    if success == false then
        return success, formatError(result)
    end
    local props = {}
    local errs = {}
    local seen = {}
    for _, prop in ipairs(result) do
        seen[prop.did] = true
        if prop.code ~= 0 then
            errs[prop.did] = ("failed to get property '%s' (code=%s)"):format(prop.did, tostring(prop.code))
        else
            props[prop.did] = encodeValue(prop.value)
        end
    end
    for _, name in ipairs(names) do
        if seen[name] ~= true and errs[name] == nil then
            errs[name] = ("missing property '%s' in response"):format(name)
        end
    end
    return success, props, errs
end

---@param obj MiioDevice
---@param name string
---@param waiter table
local function appendWaiter(obj, name, waiter)
    local waiters = obj.waiters[name]
    if waiters == nil then
        waiters = {}
        obj.waiters[name] = waiters
    end
    tinsert(waiters, waiter)
end

---@param obj MiioDevice
local function scheduleFlush(obj)
    if obj.inflight ~= nil or #obj.pending == 0 then
        return
    end
    obj.timer:start(0)
end

---@param obj MiioDevice
---@param waiter table
local function resumeWaiter(obj, waiter)
    if waiter.done == true then
        return
    end

    waiter.done = true

    if waiter.error ~= nil then
        waiter.future:reject(waiter.error)
    else
        local props = {}
        for _, name in ipairs(waiter.names) do
            props[name] = decodeValue(waiter.values[name])
        end
        waiter.future:resolve(props)
    end
end

---@param obj MiioDevice
---@param errmsg string
local function failAllWaiters(obj, errmsg)
    local resumed = {}
    for _, waiters in pairs(obj.waiters) do
        for _, waiter in ipairs(waiters) do
            if waiter.done ~= true and resumed[waiter] ~= true then
                resumed[waiter] = true
                waiter.error = errmsg
                resumeWaiter(obj, waiter)
            end
        end
    end
    obj.waiters = {}
    obj.pending = {}
    obj.pendingSet = {}
end

---@param obj MiioDevice
---@param names string[]
---@param props table<string, any>
---@param errs table<string, string>
local function deliverBatch(obj, names, props, errs)
    for _, name in ipairs(names) do
        local waiters = obj.waiters[name]
        obj.waiters[name] = nil
        if waiters ~= nil then
            local err = errs[name]
            local value = props[name]
            for _, waiter in ipairs(waiters) do
                if waiter.done ~= true and waiter.remaining[name] == true then
                    waiter.remaining[name] = nil
                    waiter.remainingCount = waiter.remainingCount - 1
                    if err ~= nil then
                        waiter.error = err
                    else
                        waiter.values[name] = value
                    end
                    if waiter.error ~= nil or waiter.remainingCount == 0 then
                        resumeWaiter(obj, waiter)
                    end
                end
            end
        end
    end
end

---Flush pending property requests.
---@param obj MiioDevice
local function flushProps(obj)
    local success, errmsg = xpcall(function ()
        if obj.inflight ~= nil or #obj.pending == 0 then
            return
        end

        local names = obj.pending
        obj.pending = {}
        obj.pendingSet = {}

        local inflight = {}
        for _, name in ipairs(names) do
            inflight[name] = true
        end
        obj.inflight = inflight

        local ok, propsOrErr, errs = obj.fetchProps(obj, names)
        obj.inflight = nil

        if ok == false then
            failAllWaiters(obj, propsOrErr)
            return
        end

        deliverBatch(obj, names, propsOrErr, errs or {})
        scheduleFlush(obj)
    end, traceback)

    if success == false then
        obj.inflight = nil
        failAllWaiters(obj, formatError(errmsg))
    end
end

---@param obj MiioDevice
---@param names string[]
---@return table<string, any>
local function fetchPropsNow(obj, names)
    local success, propsOrErr, errs = obj.fetchProps(obj, names)
    if success == false then
        obj.logger:error(propsOrErr)
        error("failed to get properties")
    end

    local props = {}
    errs = errs or {}
    for _, name in ipairs(names) do
        local err = errs[name]
        if err ~= nil then
            obj.logger:error(err)
            error("failed to get properties")
        end
        props[name] = decodeValue(propsOrErr[name])
    end
    return props
end

---Set MIOT property mapping.
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
function device:setMapping(mapping)
    self.mapping = mapping
    self.fetchProps = requestPropsMiot
end

---Get property.
---@param name string Property name.
---@return string|number|boolean value Property value.
---@nodiscard
function device:getProp(name)
    assert(type(name) == "string")
    return self:getProps({name})[name]
end

---Get properties.
---@param names string[] Property names.
---@return table<string, any> props
---@nodiscard
function device:getProps(names)
    assert(type(names) == "table")

    names = normalizeNames(names)
    if #names == 0 then
        return {}
    end

    if isYieldable() == false then
        return fetchPropsNow(self, names)
    end

    local waiter = {
        future = createFuture(),
        names = names,
        values = {},
        remaining = {},
        remainingCount = 0,
        done = false,
    }

    for _, name in ipairs(names) do
        waiter.remaining[name] = true
        waiter.remainingCount = waiter.remainingCount + 1
        appendWaiter(self, name, waiter)
        if self.inflight == nil or self.inflight[name] ~= true then
            if self.pendingSet[name] ~= true then
                self.pendingSet[name] = true
                tinsert(self.pending, name)
            end
        end
    end

    scheduleFlush(self)

    local success, result = waiter.future:wait()
    if success == false then
        self.logger:error(result)
        error("failed to get properties")
    end
    return result
end

---Set property.
---@param name string Property name.
---@param value string|number|boolean Property value.
function device:setProp(name, value)
    assert(type(name) == "string")

    if self.mapping then
        assert(self:request("set_properties", {
            did = name,
            siid = self.mapping[name].siid,
            piid = self.mapping[name].piid,
            value = value
        })[1].code == 0)
    else
        assert(self:request("set_" .. name, value)[1] == "ok")
    end
end

---Get device information.
---@return MiioDeviceInfo info
---@nodiscard
function device:getInfo()
    return self:request("miIO.info")
end

---Start a request.
---@param method string The request method.
---@param ... any The request parameters.
---@return any result
function device:request(method, ...)
    return self.pcb:request(self.timeout, method, ...)
end

---Create a device object.
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice obj Device object.
---@nodiscard
function M.create(addr, token)
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 32)

    ---@class MiioDevice
    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        pcb = protocol.create(addr, util.hex2bin(token)),
        mapping = nil,
        addr = addr,
        timeout = 1000,
        fetchProps = requestProps,
        pending = {}, ---@type string[]
        pendingSet = {}, ---@type table<string, boolean>
        inflight = nil, ---@type table<string, boolean>?
        waiters = {}, ---@type table<string, table[]>
    }

    o.timer = core.createTimer(flushProps, o)

    setmetatable(o, {
        __index = device
    })

    return o
end

return M
