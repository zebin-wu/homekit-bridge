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

local mapping = {
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
        auto = TgtHeatCoolState.value.HeatOrCool
    }
}

---Create a acpartner.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function acpartner.gen(device, info, conf)
    local iids = {
        acc = hap.getNewBridgedAccessoryID()
    }

    for i, v in ipairs({
        "heaterCooler", "active", "curTemp", "curState", "tgtState", "coolThrTemp", "heatThrTemp", "swingMode"
    }) do
        iids[v] = hap.getNewInstanceID()
    end

    device:regProps({
        "power", "mode", "tar_temp", "ver_swing"
    }, function (self, name, iids)
        if name == "power" then
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.active)
        elseif name == "mode" then
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.curState)
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.tgtState)
        elseif name == "tar_temp" then
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.coolThrTemp)
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.heatThrTemp)
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.curTemp)
        elseif name == "ver_swing" then
            hap.raiseEvent(iids.acc, iids.heaterCooler, iids.swingMode)
        end
    end, iids)

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Acpartner",
        mfg = "lumi",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = iids.heaterCooler,
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
                    Active.new(iids.active, function (request, self)
                        local value = mapping.power[self:getProp("power")]
                        self.logger:info("Read Active: " .. util.searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write Active: " .. util.searchKey(Active.value, value))
                        self:setProp("power", util.searchKey(mapping.power, value))
                        return hap.Error.None
                    end),
                    CurTemp.new(iids.curTemp, function (request, self)
                        local value = self:getProp("tar_temp")
                        self.logger:info("Read CurrentTemperature: " .. value)
                        return value, hap.Error.None
                    end),
                    CurHeatCoolState.new(iids.curState, function (request, self)
                        local mode = self:getProp("mode")
                        local value
                        if mode == "cool" then
                            value = CurHeatCoolState.value.Cooling
                        elseif mode == "heat" then
                            value = CurHeatCoolState.value.Heating
                        else
                            value = CurHeatCoolState.value.Idle
                        end
                        self.logger:info("Read CurrentHeaterCoolerState: " .. util.searchKey(CurHeatCoolState.value, value))
                        return value, hap.Error.None
                    end),
                    TgtHeatCoolState.new(iids.tgtState, function (request, self)
                        local value = mapping.mode[self:getProp("mode")]
                        self.logger:info("Read TargetHeaterCoolerState: " .. util.searchKey(TgtHeatCoolState.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write TargetHeaterCoolerState: " .. util.searchKey(TgtHeatCoolState.value, value))
                        self:setProp("mode", util.searchKey(mapping.mode, value))
                        return hap.Error.None
                    end),
                    CoolThrholdTemp.new(iids.coolThrTemp, function (request, self)
                        local value =  self:getProp("tar_temp")
                        self.logger:info("Read CoolingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write CoolingThresholdTemperature: " .. value)
                        self:setProp("tar_temp", math.tointeger(value))
                        return hap.Error.None
                    end, 16, 30, 1),
                    HeatThrholdTemp.new(iids.heatThrTemp, function (request, self)
                        local value = self:getProp("tar_temp")
                        self.logger:info("Read HeatingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write HeatingThresholdTemperature: " .. value)
                        self:setProp("tar_temp", math.tointeger(value))
                        return hap.Error.None
                    end, 16, 30, 1),
                    SwingMode.new(iids.swingMode, function (request, self)
                        local value = mapping.ver_swing[self:getProp("ver_swing")]
                        self.logger:info("Read SwingMode: " .. util.searchKey(SwingMode.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write SwingMode: " .. util.searchKey(SwingMode.value, value))
                        self:setProp("ver_swing", util.searchKey(mapping.ver_swing, value))
                        return hap.Error.None
                    end)
                }
            }
        },
        cbs = {
            identify = function (request, self)
                self.logger:info("Identify callback is called.")
                return hap.Error.None
            end
        },
        context = device
    }
end

return acpartner
