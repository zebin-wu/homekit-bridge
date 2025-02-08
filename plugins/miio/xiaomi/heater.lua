local hap = require "hap"
local Active = require "hap.char.Active"
local CurTemp = require "hap.char.CurrentTemperature"
local CurHeatCoolState = require "hap.char.CurrentHeaterCoolerState"
local TgtHeatCoolState = require "hap.char.TargetHeaterCoolerState"
local HeatThrholdTemp = require "hap.char.HeatingThresholdTemperature"
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local M = {}

---Create a acpartner.
---@param device MiioDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Xiaomi Heater",
        "xiaomi",
        conf.model,
        conf.sn,
        conf.fw_ver,
        conf.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.heater, "HeaterCooler", true, false, {
                Active.new(iids.active, function (request)
                    return device:getProp("on") and Active.value.Active or Active.value.Inactive
                end, function (request, value)
                    device:setProp("on", value == Active.value.Active)
                    raiseEvent(request.aid, request.sid, request.cid)
                    core.createTimer(function ()
                        raiseEvent(request.aid, iids.heaterCooler, iids.curState)
                    end):start(500)
                end),
                CurTemp.new(iids.curTemp, function (request)
                    return device:getProp("curTemp")
                end):setContraints(-30, 100, 1),
                CurHeatCoolState.new(iids.curState, function (request)
                    if not device:getProp("on") then
                        return CurHeatCoolState.value.Inactive
                    end
                    return device:getProp("curState") == 2 and CurHeatCoolState.value.Heating or CurHeatCoolState.value.Idle
                end):setValidVals(CurHeatCoolState.value.Inactive, CurHeatCoolState.value.Idle, CurHeatCoolState.value.Heating),
                TgtHeatCoolState.new(iids.tgtState, function (request)
                    return TgtHeatCoolState.value.Heat
                end, nil):setValidVals(TgtHeatCoolState.value.Heat),
                HeatThrholdTemp.new(iids.heatThrTemp, function (request)
                    return device:getProp("tgtTemp")
                end, function (request, value)
                    device:setProp("tgtTemp", assert(tointeger(value), "value not a integer"))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(18, 28, 1),
            })
        },
        function (request)
            device.logger:info("Identify callback is called.")
        end
    )
end

return M
