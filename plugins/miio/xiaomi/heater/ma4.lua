local M = {}

---Create a dehumidifier.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    -- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:heater:0000A01A:xiaomi-ma4:1
    device:setMapping({
        on = {siid = 2, piid = 1},
        tgtTemp = {siid = 2, piid = 5},
        curTemp = {siid = 2, piid = 6},
        curState = {siid = 2, piid = 3},
    })

    return require("miio.xiaomi.heater").gen(device, conf)
end

return M
