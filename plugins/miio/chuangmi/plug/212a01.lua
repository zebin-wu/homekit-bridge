local plug = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf)
    device:setMapping({
        power = {siid = 2, piid = 1}
    })

    function device:getOn()
        return self:getProp("power")
    end

    function device:setOn(value)
        self:setProp("power", value)
    end

    return require("miio.chuangmi.plug").gen(device, info, conf)
end

return plug
