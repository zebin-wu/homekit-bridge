local hap = require "hap"
local Active = require "hap.char.Active"
local CurrentState = require "hap.char.CurrentHumidifierDehumidifierState"
local TargetState = require "hap.char.TargetHumidifierDehumidifierState"
local CurrentHumidity = require "hap.char.CurrentRelativeHumidity"
local TargetHumidity = require "hap.char.RelativeHumidityDehumidifierThreshold"
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local M = {}

---Create a dehumidifier.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Dmaker Dehumidifier",
        "dmaker",
        info.model,
        conf.sn,
        info.fw_ver,
        info.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.derh, "HumidifierDehumidifier", true, false, {
                Active.new(iids.active, function (request)
                    return device:getProp("power") and Active.value.Active or Active.value.Inactive
                end, function (request, value)
                    device:setProp("power", value == Active.value.Active)
                    raiseEvent(request.aid, request.sid, request.cid)
                    raiseEvent(request.aid, request.sid, iids.curState)
                end),
                CurrentState.new(iids.curState, function (request)
                    return device:getProp("power") and CurrentState.value.Dehumidifying or CurrentState.value.Inactive
                end):setValidVals(CurrentState.value.Inactive, CurrentState.value.Dehumidifying),
                TargetState.new(iids.tgtState, function (request)
                    return TargetState.value.Dehumidifier
                end, nil):setValidVals(TargetState.value.Dehumidifier),
                CurrentHumidity.new(iids.curHumidity, function (request)
                    return device:getProp("curHumidity")
                end),
                TargetHumidity.new(iids.tgtHumidity, function (request)
                    return device:getProp("tgtHumidity")
                end, function (request, value)
                    device:setProp("tgtHumidity", assert(tointeger(value), "value not a integer"))
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
