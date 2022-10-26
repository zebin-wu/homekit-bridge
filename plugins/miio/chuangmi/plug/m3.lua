local M = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    function device:getOn()
        return self:getProp("power") == "on"
    end

    function device:setOn(value)
        local power
        if value == true then
            power = "on"
        else
            power = "off"
        end
        self:setProp("power", power)
    end

    return require("miio.plug").gen(device, info, conf)
end

return M
