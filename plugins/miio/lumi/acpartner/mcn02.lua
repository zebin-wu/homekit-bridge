local hap = require "hap"
local Active = require "hap.char.Active"
local CurTemp = require "hap.char.CurrentTemperature"
local CurHeatCoolState = require "hap.char.CurrentHeaterCoolerState"
local TgtHeatCoolState = require "hap.char.TargetHeaterCoolerState"
local CoolThrholdTemp = require "hap.char.CoolingThresholdTemperature"
local HeatThrholdTemp = require "hap.char.HeatingThresholdTemperature"
local SwingMode = require "hap.char.SwingMode"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local M = {}

--- Property value -> Characteristic value.
local valMapping = {
    power = {
        on = Active.value.Active,
        off = Active.value.Inactive
    },
    ver_swing = {
        on = SwingMode.value.Enabled,
        off = SwingMode.value.Disabled
    },
    mode = {
        cool = TgtHeatCoolState.value.Cool,
        heat = TgtHeatCoolState.value.Heat,
        auto = TgtHeatCoolState.value.HeatOrCool,
    }
}

---Create a acpartner.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Lumi Acpartner",
        "lumi",
        info.model,
        conf.sn,
        info.fw_ver,
        info.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.heaterCooler, "HeaterCooler", true, false, {
                Active.new(iids.active, function (request)
                    return valMapping.power[device:getProp("power")]
                end, function (request, value)
                    device:setProp("power", searchKey(valMapping.power, value))
                    raiseEvent(request.aid, request.sid, request.cid)
                    core.createTimer(function ()
                        raiseEvent(request.aid, iids.heaterCooler, iids.curTemp)
                        raiseEvent(request.aid, iids.heaterCooler, iids.tgtState)
                        raiseEvent(request.aid, iids.heaterCooler, iids.curState)
                        raiseEvent(request.aid, iids.heaterCooler, iids.coolThrTemp)
                        raiseEvent(request.aid, iids.heaterCooler, iids.heatThrTemp)
                        raiseEvent(request.aid, iids.heaterCooler, iids.swingMode)
                    end):start(500)
                end),
                CurTemp.new(iids.curTemp, function (request)
                    return device:getProp("tar_temp")
                end),
                CurHeatCoolState.new(iids.curState, function (request)
                    local mode = device:getProp("mode")
                    local value
                    if mode == "cool" then
                        value = CurHeatCoolState.value.Cooling
                    elseif mode == "heat" then
                        value = CurHeatCoolState.value.Heating
                    else
                        value = CurHeatCoolState.value.Idle
                    end
                    return value
                end),
                TgtHeatCoolState.new(iids.tgtState, function (request)
                    local mode = device:getProp("mode")
                    local value
                    if mode == "unsupport" or mode == "dry" or mode == "wind" then
                        value = TgtHeatCoolState.value.HeatOrCool
                    else
                        value = valMapping.mode[mode]
                    end
                    return value
                end, function (request, value)
                    device:setProp("mode", searchKey(valMapping.mode, value))
                    raiseEvent(request.aid, request.sid, request.cid)
                    core.createTimer(function ()
                        raiseEvent(request.aid, iids.heaterCooler, iids.curState)
                        raiseEvent(request.aid, iids.heaterCooler, iids.coolThrTemp)
                        raiseEvent(request.aid, iids.heaterCooler, iids.heatThrTemp)
                    end):start(500)
                end),
                CoolThrholdTemp.new(iids.coolThrTemp, function (request)
                    return device:getProp("tar_temp")
                end, function (request, value)
                    device:setProp("tar_temp", assert(tointeger(value), "value not a integer"))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(16, 30, 1),
                HeatThrholdTemp.new(iids.heatThrTemp, function (request)
                    return device:getProp("tar_temp")
                end, function (request, value)
                    device:setProp("tar_temp", assert(tointeger(value), "value not a integer"))
                    raiseEvent(request.aid, request.sid, request.cid)
                end):setContraints(16, 30, 1),
                SwingMode.new(iids.swingMode, function (request)
                    local ver_swing = device:getProp("ver_swing")
                    local value
                    if ver_swing == "unsupport" then
                        value = SwingMode.value.Disabled
                    else
                        value = valMapping.ver_swing[ver_swing]
                    end
                    return value
                end, function (request, value)
                    device:setProp("ver_swing", searchKey(valMapping.ver_swing, value))
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
