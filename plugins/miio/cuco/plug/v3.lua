local M = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    -- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:outlet:0000A002:cuco-v3:1
    device:setMapping({
        power = {siid = 2, piid = 1}
    })

    function device:getOn()
        return self:getProp("power")
    end

    function device:setOn(value)
        self:setProp("power", value)
    end

    return require("miio.plug").gen(device, info, conf)
end

return M
