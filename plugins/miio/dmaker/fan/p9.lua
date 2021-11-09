local fan = {}

---Create a fan.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function fan.gen(device, info, conf)
    return require("miio.dmaker.fan").gen(device, info, conf, {
        power = {siid = 2, piid = 1},
        fanSpeed = {siid = 2, piid = 11},
        swingMode = {siid = 2, piid = 5}
    })
end

return fan
