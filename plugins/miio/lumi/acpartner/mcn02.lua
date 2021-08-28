local device = require "miio.device"
local hap = require "hap"
local util = require "util"
local Name = require "hap.char.Name"
local Active = require "hap.char.Active"
local CurTemp = require "hap.char.CurTemp"
local CurHeatCoolState = require "hap.char.CurHeatCoolState"
local TgtHeatCoolState = require "hap.char.TgtHeatCoolState"
local CoolThrholdTemp = require "hap.char.CoolThrholdTemp"
local HeatThrholdTemp = require "hap.char.HeatThrholdTemp"

local acpartner = {}
local model = "lumi.acpartner.mcn02"
local logger = log.getLogger(model)

---Create acpartner.
---@param obj MiioDevice Device object.
---@return Accessory accessory HomeKit Accessory.
function acpartner.gen(obj, conf)
    obj:syncProps({
        "power", "mode", "tar_temp", "far_level", "ver_swing"
    })

    return {
        aid = hap.getNewBridgedAccessoryID(),
        category = "BridgedAccessory",
        name = conf.name or "Acparnter",
        mfg = "lumi",
        model = model,
        sn = obj.info.mac,
        fwVer = obj.info.fw_ver,
        hwVer = obj.info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = hap.getNewInstanceID(),
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
                    Name.new(hap.getNewInstanceID()),
                    Active.new(hap.getNewInstanceID(), function (request, obj)
                        local value
                        if obj:getProp("power") == "on" then
                            value = Active.value.Active
                        else
                            value = Active.value.Inactive
                        end
                        logger:info("Read active: " .. util.searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, obj)
                        logger:info("Write active: " .. util.searchKey(Active.value, value))
                        local power
                        local changed = false
                        if value == Active.value.Active then
                            power = "on"
                        else
                            power = "off"
                        end
                        if obj:getProp("power") ~= power then
                            obj:setProp("power", power)
                            changed = true
                        end
                        return changed, hap.Error.None
                    end),
                    CurTemp.new(hap.getNewInstanceID(), function (request, obj)
                        local value = obj:getProp("tar_temp")
                        logger:info("Read CurrentTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end),
                    CurHeatCoolState.new(hap.getNewInstanceID(), function (request, obj)
                        local mode = obj:getProp("mode")
                        local value
                        if mode == "cool" then
                            value = CurHeatCoolState.value.Cooling
                        elseif mode == "heat" then
                            value = CurHeatCoolState.value.Heating
                        end
                        logger:info("Read CurrentHeaterCoolerState: " .. util.searchKey(CurHeatCoolState.value, value))
                        return value, hap.Error.None
                    end),
                    TgtHeatCoolState.new(hap.getNewInstanceID(), function (request, obj)
                        local mode = obj:getProp("mode")
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
                    end, function (request, value, obj)
                        logger:info("Write TargetHeaterCoolerState: " .. util.searchKey(TgtHeatCoolState.value, value))
                        local mode
                        local changed = false
                        if value == TgtHeatCoolState.value.Cool then
                            mode = "cool"
                        elseif value == TgtHeatCoolState.value.Heat then
                            mode = "heat"
                        elseif value == TgtHeatCoolState.value.HeatOrCool then
                            mode = "auto"
                        end
                        if obj:getProp("mode") ~= mode then
                            obj:setProp("mode", mode)
                            changed = true
                        end
                        return changed, hap.Error.None
                    end),
                    CoolThrholdTemp.new(hap.getNewInstanceID(), function (request, obj)
                        if obj:getProp("mode") ~= "cool" then
                            return 25, hap.Error.None
                        end
                        local value =  obj:getProp("tar_temp")
                        logger:info("Read CoolingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, obj)
                        local changed = false
                        value = math.tointeger(value)
                        logger:info("Write CoolingThresholdTemperature: " .. value)
                        if obj:getProp("tar_temp") ~= value then
                            obj:setProp("tar_temp", value)
                            changed = true
                        end
                        return changed, hap.Error.None
                    end, 16, 30, 1),
                    HeatThrholdTemp.new(hap.getNewInstanceID(), function (request, obj)
                        if obj:getProp("mode") ~= "heat" then
                            return 0, hap.Error.None
                        end
                        local value = obj:getProp("tar_temp")
                        logger:info("Read HeatingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, obj)
                        local changed = false
                        value = math.tointeger(value)
                        logger:info("Write HeatingThresholdTemperature: " .. value)
                        if obj:getProp("tar_temp") ~= value then
                            obj:setProp("tar_temp", value)
                            changed = true
                        end
                        return changed, hap.Error.None
                    end, 16, 30, 1),
                }
            }
        },
        cbs = {
            identify = function (request, context)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        },
        context = obj
    }
end

return acpartner
