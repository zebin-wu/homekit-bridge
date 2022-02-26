local derh = {}

---Create a dehumidifier.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function derh.gen(device, info, conf)
    -- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:dehumidifier:0000A02D:dmaker-22l:1
    device:setMapping({
        power = {siid = 2, piid = 1},
        tgtHumidity = {siid = 2, piid = 5},
        curHumidity = {siid = 3, piid = 1},
    })

    return require("miio.dmaker.derh").gen(device, info, conf)
end

return derh
