local protocol = require "miio.protocol"

local assert = assert
local type = type

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

---Set MIOT property mapping.
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
function _device:setMapping(mapping)
    self.mapping = mapping
end

---Get property.
---@param name string Property name.
---@return string|number|boolean value Property value.
---@nodiscard
function _device:getProp(name)
    if self.mapping then
        local result = self:request("get_properties", {{
            did = name,
            siid = self.mapping[name].siid,
            piid = self.mapping[name].piid,
        }})
        assert(#result == 1 and result[1])
        return result[1].value
    else
        local result = self:request("get_prop", {name})
        assert(#result == 1)
        return result[1]
    end
end

---Set property.
---@param name string Property name.
---@param value string|number|boolean Property value.
function _device:setProp(name, value)
    if self.mapping then
        self:request("set_properties", {{
            did = name,
            siid = self.mapping[name].siid,
            piid = self.mapping[name].piid,
            value = value
        }})
    else
        self:request("set_" .. name, {value})
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
    }

    setmetatable(o, {
        __index = _device
    })

    return o
end

return device
