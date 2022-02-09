local plug = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf)
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

    return require("miio.chuangmi.plug").gen(device, info, conf, "power")
end

return plug
