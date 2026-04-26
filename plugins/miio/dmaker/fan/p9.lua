local M = {}

-- Source https://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:fan:0000A005:dmaker-p9:1
local propMapping = {
    power = {siid = 2, piid = 1},
    fanSpeed = {siid = 2, piid = 11},
    swingMode = {siid = 2, piid = 5}
}

---Create a fan.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    device:setMapping(propMapping)

    return require("miio.dmaker.fan").gen(device, conf)
end

return M
