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

    return {
        aid = conf.aid,
        category = "BridgedAccessory",
        name = conf.name or "Lumi Acpartner",
        mfg = "lumi",
        model = info.model,
        sn = conf.sn,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.heaterCooler,
                type = "HeaterCooler",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    Active.new(iids.active, function (request)
                        local value = valMapping.power[device:getProp("power")]
                        device.logger:info("Read Active: " .. searchKey(Active.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write Active: " .. searchKey(Active.value, value))
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
                        local value = device:getProp("tar_temp")
                        device.logger:info("Read CurrentTemperature: " .. value)
                        return value
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
                        device.logger:info("Read CurrentHeaterCoolerState: " .. searchKey(CurHeatCoolState.value, value))
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
                        device.logger:info("Read TargetHeaterCoolerState: " .. searchKey(TgtHeatCoolState.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write TargetHeaterCoolerState: " .. searchKey(TgtHeatCoolState.value, value))
                        device:setProp("mode", searchKey(valMapping.mode, value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        core.createTimer(function ()
                            raiseEvent(request.aid, iids.heaterCooler, iids.curState)
                            raiseEvent(request.aid, iids.heaterCooler, iids.coolThrTemp)
                            raiseEvent(request.aid, iids.heaterCooler, iids.heatThrTemp)
                        end):start(500)
                    end),
                    CoolThrholdTemp.new(iids.coolThrTemp, function (request)
                        local value =  device:getProp("tar_temp")
                        device.logger:info("Read CoolingThresholdTemperature: " .. value)
                        return value
                    end, function (request, value)
                        device.logger:info("Write CoolingThresholdTemperature: " .. value)
                        device:setProp("tar_temp", assert(tointeger(value), "value not a integer"))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end, 16, 30, 1),
                    HeatThrholdTemp.new(iids.heatThrTemp, function (request)
                        local value = device:getProp("tar_temp")
                        device.logger:info("Read HeatingThresholdTemperature: " .. value)
                        return value
                    end, function (request, value)
                        device.logger:info("Write HeatingThresholdTemperature: " .. value)
                        device:setProp("tar_temp", assert(tointeger(value), "value not a integer"))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end, 16, 30, 1),
                    SwingMode.new(iids.swingMode, function (request)
                        local ver_swing = device:getProp("ver_swing")
                        local value
                        if ver_swing == "unsupport" then
                            value = SwingMode.value.Disabled
                        else
                            value = valMapping.ver_swing[ver_swing]
                        end
                        device.logger:info("Read SwingMode: " .. searchKey(SwingMode.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write SwingMode: " .. searchKey(SwingMode.value, value))
                        device:setProp("ver_swing", searchKey(valMapping.ver_swing, value))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end)
                }
            }
        },
        cbs = {
            identify = function (request)
                device.logger:info("Identify callback is called.")
            end
        }
    }
end

return M
