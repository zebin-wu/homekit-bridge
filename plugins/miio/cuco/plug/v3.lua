local M = {}

-- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:outlet:0000A002:cuco-v3:1
local propMapping = {
    power = {siid = 2, piid = 1}
}

---Create a plug.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    device:setMapping(propMapping)

    function device:getOn()
        return self:getProp("power")
    end

    function device:setOn(value)
        self:setProp("power", value)
    end

    return require("miio.plug").gen(device, conf)
end

return M
