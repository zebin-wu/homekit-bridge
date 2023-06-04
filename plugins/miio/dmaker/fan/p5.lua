local hap = require "hap"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"
local tointeger = math.tointeger
local raiseEvent = hap.raiseEvent

local M = {}

---Create a fan.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Dmaker Fan",
        "dmaker",
        conf.model,
        conf.sn,
        conf.fw_ver,
        conf.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.fan, "Fan", true, false, {
                Active.new(iids.active, function (request)
                    return device:getProp("power") and Active.value.Active or Active.value.Inactive
                end, function (request, value)
                    device:s_power(value == Active.value.Active)
                    raiseEvent(request.aid, request.sid, request.cid)
                end),
                RotationSpeed.new(iids.rotationSpeed, function (request)
                    return device:getProp("speed")
                end, function (request, value)
                    device:s_speed(tointeger(value))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(1, 100, 1),
                SwingMode.new(iids.swingMode, function (request)
                    return device:getProp("roll_enable") and SwingMode.value.Enabled or SwingMode.value.Disabled
                end, function (request, value)
                    device:s_roll(value == SwingMode.value.Enabled)
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
