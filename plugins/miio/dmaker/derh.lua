local hap = require "hap"
local Active = require "hap.char.Active"
local CurState = require "hap.char.CurrentHumidifierDehumidifierState"
local TgtState = require "hap.char.TargetHumidifierDehumidifierState"
local CurHumidity = require "hap.char.CurrentRelativeHumidity"
local TgtHumidity = require "hap.char.RelativeHumidityDehumidifierThreshold"
local CurTemp = require "hap.char.CurrentTemperature"
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local M = {}

---Create a dehumidifier.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Dmaker Dehumidifier",
        "dmaker",
        conf.model,
        conf.sn,
        conf.fw_ver,
        conf.hw_ver,
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
                CurState.new(iids.curState, function (request)
                    return device:getProp("power") and CurState.value.Dehumidifying or CurState.value.Inactive
                end):setValidVals(CurState.value.Inactive, CurState.value.Dehumidifying),
                TgtState.new(iids.tgtState, function (request)
                    return TgtState.value.Dehumidifier
                end, nil):setValidVals(TgtState.value.Dehumidifier),
                CurHumidity.new(iids.curHumidity, function (request)
                    return device:getProp("curHumidity")
                end),
                TgtHumidity.new(iids.tgtHumidity, function (request)
                    return device:getProp("tgtHumidity")
                end, function (request, value)
                    device:setProp("tgtHumidity", assert(tointeger(value), "value not a integer"))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(30, 70, 1)
            }),
            hap.newService(iids.temp, "TemperatureSensor", false, false, {
                CurTemp.new(iids.curTemp, function (request)
                    return device:getProp("curTemp")
                end):setContraints(-30, 100, 0.1)
            })
        },
        function (request)
            device.logger:info("Identify callback is called.")
        end
    )
end

return M
