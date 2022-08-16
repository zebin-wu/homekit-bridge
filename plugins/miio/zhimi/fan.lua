local hap = require "hap"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local M = {}

--- Property value -> Characteristic value.
local valMapping = {
    power = {
        on = Active.value.Active,
        off = Active.value.Inactive,
    },
    angle_enable = {
        on = SwingMode.value.Enabled,
        off = SwingMode.value.Disabled
    }
}

---Create a fan.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Zhimi Fan",
        "zhimi",
        info.model,
        conf.sn,
        info.fw_ver,
        info.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.fan, "Fan", true, false, {
                Active.new(iids.active, function (request)
                    return valMapping.power[device:getProp("power")]
                end, function (request, value)
                    device:setProp("power", searchKey(valMapping.power, value))
                    raiseEvent(request.aid, request.sid, request.cid)
                end),
                RotationSpeed.new(iids.rotationSpeed, function (request)
                    return device:getProp("speed_level")
                end, function (request, value)
                    device:setProp("speed_level", assert(tointeger(value), "value not a integer"))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(1, 100, 1),
                SwingMode.new(iids.swingMode, function (request)
                    return valMapping.angle_enable[device:getProp("angle_enable")]
                end, function (request, value)
                    device:setProp("angle_enable", searchKey(valMapping.angle_enable, value))
                    raiseEvent(request.aid, request.sid, request.cid)
                end)
            })
        },
        function (request)
            device.logger:info("Identify callback is called.")
        end
    )
end

return M
