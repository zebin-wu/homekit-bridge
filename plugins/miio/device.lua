local protocol = require "miio.protocol"
local util = require "util"
local xpcall = xpcall
local traceback = debug.traceback
local assert = assert
local type = type
local tunpack = table.unpack
local tinsert = table.insert

local M = {}

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

---Get properties.
---@param obj MiioDevice
local function getProps(obj)
    local names = obj.names
    obj.names = {}
    local success, result = xpcall(obj.request, traceback, obj, "get_prop", tunpack(names))
    if success == false then
        obj.mq:send(success, result)
        return
    end
    local props = {}
    for i, value in ipairs(result) do
        props[names[i]] = value
    end
    obj.mq:send(success, props)
end

---Get properties(MIOT).
---@param obj MiioDevice
local function getPropsMiot(obj)
    local mapping = obj.mapping
    assert(mapping ~= nil, "missing mapping")
    local params = {}
    for _, name in ipairs(obj.names) do
        tinsert(params, {
            did = name,
            siid = mapping[name].siid,
            piid = mapping[name].piid,
        })
    end
    obj.names = {}
    local success, result = xpcall(obj.request, traceback, obj, "get_properties", tunpack(params))
    if success == false then
        obj.mq:send(success, result)
        return
    end
    local props = {}
    for _, prop in ipairs(result) do
        props[prop.did] = prop.value
    end
    obj.mq:send(success, props)
end

---Set MIOT property mapping.
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
function device:setMapping(mapping)
    self.mapping = mapping
    self.timer = core.createTimer(getPropsMiot, self)
end

---Get property.
---@param name string Property name.
---@return string|number|boolean value Property value.
---@nodiscard
function device:getProp(name)
    assert(type(name) == "string")

    local names = self.names
    if #names == 0 then
        self.timer:start(0)
    else
        for _, _name in ipairs(names) do
            if name == _name then
                goto recv
            end
        end
    end
    tinsert(names, name)

::recv::
    local success, result = self.mq:recv()
    if success == false then
        self.logger:error(result)
        error("failed to get property")
    end
    if result[name] == nil then
        goto recv
    end
    return result[name]
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
        mapping = false,
        addr = addr,
        timeout = 1000,
        mq = core.createMQ(1),
        names = {}, ---@type string[]
    }

    o.timer = core.createTimer(getProps, o)

    setmetatable(o, {
        __index = device
    })

    return o
end

return M
