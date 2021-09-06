local hap = require "hap"
local util = require "util"
local Active = require "hap.char.Active"
local CurTemp = require "hap.char.CurrentTemperature"
local CurHeatCoolState = require "hap.char.CurrentHeaterCoolerState"
local TgtHeatCoolState = require "hap.char.TargetHeaterCoolerState"
local CoolThrholdTemp = require "hap.char.CoolingThresholdTemperature"
local HeatThrholdTemp = require "hap.char.HeatingThresholdTemperature"
local SwingMode = require "hap.char.SwingMode"

local acpartner = {}
local model = "lumi.acpartner.mcn02"
local logger = log.getLogger(model)

---Create a acpartner.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function acpartner.gen(device, info, conf)
    local iids = {}

    for i, v in ipairs({
        "acc", "serv", "active", "curTemp", "curState", "tgtState", "coolThrTemp", "heatThrTemp", "swingMode"
    }) do
        iids[v] = hap.getNewInstanceID()
    end

    device:registerProps({
        "power", "mode", "tar_temp", "far_level", "ver_swing"
    }, function (self, name, iids)
        if name == "power" then
            hap.raiseEvent(iids.acc, iids.serv, iids.active)
        elseif name == "mode" then
            hap.raiseEvent(iids.acc, iids.serv, iids.curState)
            hap.raiseEvent(iids.acc, iids.serv, iids.tgtState)
        elseif name == "tar_temp" then
            hap.raiseEvent(iids.acc, iids.serv, iids.coolThrTemp)
            hap.raiseEvent(iids.acc, iids.serv, iids.heatThrTemp)
            hap.raiseEvent(iids.acc, iids.serv, iids.curTemp)
        elseif name == "ver_swing" then
            hap.raiseEvent(iids.acc, iids.serv, iids.swingMode)
        end
    end, iids)

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Acpartner",
        mfg = "lumi",
        model = model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = iids.serv,
                type = "HeaterCooler",
                name = "Heater Cooler",
                props = {
                    primaryService = true,
                    hidden = false,
                    ble = {
                        supportsConfiguration = false,
                    }
                },
                chars = {
                    Active.new(iids.active, function (request, device)
                        local value
                        if device:getProp("power") == "on" then
                            value = Active.value.Active
                        else
                            value = Active.value.Inactive
                        end
                        logger:info("Read active: " .. util.searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, device)
                        logger:info("Write active: " .. util.searchKey(Active.value, value))
                        local power
                        if value == Active.value.Active then
                            power = "on"
                        else
                            power = "off"
                        end
                        device:setProp("power", power)
                        return hap.Error.None
                    end),
                    CurTemp.new(iids.curTemp, function (request, device)
                        local value = device:getProp("tar_temp")
                        logger:info("Read CurrentTemperature: " .. value)
                        return value, hap.Error.None
                    end),
                    CurHeatCoolState.new(iids.curState, function (request, device)
                        local mode = device:getProp("mode")
                        local value
                        if mode == "cool" then
                            value = CurHeatCoolState.value.Cooling
                        elseif mode == "heat" then
                            value = CurHeatCoolState.value.Heating
                        else
                            value = CurHeatCoolState.value.Idle
                        end
                        logger:info("Read CurrentHeaterCoolerState: " .. util.searchKey(CurHeatCoolState.value, value))
                        return value, hap.Error.None
                    end),
                    TgtHeatCoolState.new(iids.tgtState, function (request, device)
                        local mode = device:getProp("mode")
                        local value
                        if mode == "cool" then
                            value = TgtHeatCoolState.value.Cool
                        elseif mode == "heat" then
                            value = TgtHeatCoolState.value.Heat
                        elseif mode == "auto" then
                            value = TgtHeatCoolState.value.HeatOrCool
                        end
                        logger:info("Read TargetHeaterCoolerState: " .. util.searchKey(TgtHeatCoolState.value, value))
                        return value, hap.Error.None
                    end, function (request, value, device)
                        logger:info("Write TargetHeaterCoolerState: " .. util.searchKey(TgtHeatCoolState.value, value))
                        local mode
                        if value == TgtHeatCoolState.value.Cool then
                            mode = "cool"
                        elseif value == TgtHeatCoolState.value.Heat then
                            mode = "heat"
                        elseif value == TgtHeatCoolState.value.HeatOrCool then
                            mode = "auto"
                        end
                        device:setProp("mode", mode)
                        return hap.Error.None
                    end),
                    CoolThrholdTemp.new(iids.coolThrTemp, function (request, device)
                        local value =  device:getProp("tar_temp")
                        logger:info("Read CoolingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, device)
                        logger:info("Write CoolingThresholdTemperature: " .. value)
                        device:setProp("tar_temp", math.tointeger(value))
                        return hap.Error.None
                    end, 16, 30, 1),
                    HeatThrholdTemp.new(iids.heatThrTemp, function (request, device)
                        local value = device:getProp("tar_temp")
                        logger:info("Read HeatingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, device)
                        logger:info("Write HeatingThresholdTemperature: " .. value)
                        device:setProp("tar_temp", math.tointeger(value))
                        return hap.Error.None
                    end, 16, 30, 1),
                    SwingMode.new(iids.swingMode, function (request, device)
                        local value
                        if device:getProp("ver_swing") == "on" then
                            value = SwingMode.value.Enabled
                        else
                            value = SwingMode.value.Disabled
                        end
                        logger:info("Read SwingMode: " .. util.searchKey(SwingMode.value, value))
                        return value, hap.Error.None
                    end, function (request, value, device)
                        logger:info("Write SwingMode: " .. util.searchKey(SwingMode.value, value))
                        local mode
                        if value == SwingMode.value.Enabled then
                            mode = "on"
                        else
                            mode = "off"
                        end
                        device:setProp("ver_swing", mode)
                        return hap.Error.None
                    end)
                }
            }
        },
        cbs = {
            identify = function (request, context)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        },
        context = device
    }
end

return acpartner
