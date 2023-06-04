local M = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    -- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:outlet:0000A002:chuangmi-212a01:1
    device:setMapping({
        power = {siid = 2, piid = 1}
    })

    function device:getOn()
        return self:getProp("power")
    end

    function device:setOn(value)
        self:setProp("power", value)
    end

    return require("miio.plug").gen(device, conf)
end

return M
