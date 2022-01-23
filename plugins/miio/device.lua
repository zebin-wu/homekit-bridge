local protocol = require "miio.protocol"
local time = require "time"
local mq = require "mq"
local xpcall = xpcall
local traceback = debug.traceback
local assert = assert
local type = type
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

---@class MiioDevice:MiioDevicePriv Device object.
local _device = {}

---Start a request.
---@param method string The request method.
---@param params? table Array of parameters.
---@return any result
function _device:request(method, params)
    return self.pcb:request(self.timeout, method, params)
end

---Get properties.
---@param obj MiioDevice
local function getProps(obj)
    local names = obj.names
    obj.names = {}
    local success, result = xpcall(obj.request, traceback, obj, "get_prop", names)
    if not success then
        obj.mq:send(success, result)
    end
    local props = {}
    for i, value in ipairs(result) do
        props[names[i]] = value
    end
    obj.mq:send(success, props)
end

local function getPropsMiot(obj)
    local mapping = obj.mapping
    local params = {}
    for _, name in ipairs(obj.names) do
        tinsert(params, {
            did = name,
            siid = mapping[name].siid,
            piid = mapping[name].piid,
        })
    end
    obj.names = {}
    local success, result = xpcall(obj.request, traceback, obj, "get_properties", params)
    if not success then
        obj.mq:send(success, result)
    end
    local props = {}
    for _, prop in ipairs(result) do
        props[prop.did] = prop.value
    end
    obj.mq:send(success, props)
end

---Set MIOT property mapping.
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
function _device:setMapping(mapping)
    self.mapping = mapping
    self.timer = time.createTimer(getPropsMiot, self)
end

---Get property.
---@param name string Property name.
---@return string|number|boolean value Property value.
---@nodiscard
function _device:getProp(name)
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
    if not success then
        error(result)
    end
    if result[name] == nil then
        goto recv
    end
    return result[name]
end

---Set property.
---@param name string Property name.
---@param value string|number|boolean Property value.
function _device:setProp(name, value)
    assert(type(name) == "string")

    if self.mapping then
        assert(self:request("set_properties", {{
            did = name,
            siid = self.mapping[name].siid,
            piid = self.mapping[name].piid,
            value = value
        }})[1].code == 0)
    else
        assert(self:request("set_" .. name, {value})[1] == "ok")
    end
end

---Get device information.
---@return MiioDeviceInfo info
---@nodiscard
function _device:getInfo()
    return self:request("miIO.info")
end

---Create a device object.
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice obj Device object.
---@nodiscard
function device.create(addr, token)
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class MiioDevicePriv
    local o = {
        logger = log.getLogger("miio.device:" .. addr),
        pcb = protocol.create(addr, token),
        addr = addr,
        token = token,
        timeout = 5000,
        mq = mq.create(1),
        names = {}, ---@type string[]
    }

    o.timer = time.createTimer(getProps, o)

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
