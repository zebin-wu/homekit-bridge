local hap = require "hap"
local time = require "time"
local Active = require "hap.char.Active"
local CurTemp = require "hap.char.CurrentTemperature"
local CurHeatCoolState = require "hap.char.CurrentHeaterCoolerState"
local TgtHeatCoolState = require "hap.char.TargetHeaterCoolerState"
local CoolThrholdTemp = require "hap.char.CoolingThresholdTemperature"
local HeatThrholdTemp = require "hap.char.HeatingThresholdTemperature"
local SwingMode = require "hap.char.SwingMode"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent

local acpartner = {}

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
---@return HapAccessory accessory HomeKit Accessory.
function acpartner.gen(device, info, conf)
    ---@class AcpartnerIIDS:table Acpartner Instance ID table.
    local iids = {
        acc = conf.aid,
        heaterCooler = hap.getNewInstanceID(),
        active = hap.getNewInstanceID(),
        curTemp = hap.getNewInstanceID(),
        curState = hap.getNewInstanceID(),
        tgtState = hap.getNewInstanceID(),
        coolThrTemp = hap.getNewInstanceID(),
        heatThrTemp = hap.getNewInstanceID(),
        swingMode = hap.getNewInstanceID()
    }
    device.iids = iids

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Lumi Acpartner",
        mfg = "lumi",
        model = info.model,
        sn = info.mac,
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
                    Active.new(iids.active, function (request, self)
                        local value = valMapping.power[self:getProp("power")]
                        self.logger:info("Read Active: " .. searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write Active: " .. searchKey(Active.value, value))
                        self:setProp("power", searchKey(valMapping.power, value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        time.createTimer(function (iids)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.curTemp)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.curState)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.coolThrTemp)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.heatThrTemp)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.swingMode)
                        end, self.iids):start(500)
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
                        self.logger:info("Read CurrentHeaterCoolerState: " .. searchKey(CurHeatCoolState.value, value))
                        return value, hap.Error.None
                    end),
                    TgtHeatCoolState.new(iids.tgtState, function (request, self)
                        local mode = self:getProp("mode")
                        local value
                        if mode == "unsupport" or mode == "dry" or mode == "wind" then
                            value = TgtHeatCoolState.value.HeatOrCool
                        else
                            value = valMapping.mode[mode]
                        end
                        self.logger:info("Read TargetHeaterCoolerState: " .. searchKey(TgtHeatCoolState.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write TargetHeaterCoolerState: " .. searchKey(TgtHeatCoolState.value, value))
                        self:setProp("mode", searchKey(valMapping.mode, value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        time.createTimer(function (iids)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.curState)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.coolThrTemp)
                            raiseEvent(iids.acc, iids.heaterCooler, iids.heatThrTemp)
                        end, self.iids):start(500)
                        return hap.Error.None
                    end),
                    CoolThrholdTemp.new(iids.coolThrTemp, function (request, self)
                        local value =  self:getProp("tar_temp")
                        self.logger:info("Read CoolingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write CoolingThresholdTemperature: " .. value)
                        self:setProp("tar_temp", math.tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end, 16, 30, 1),
                    HeatThrholdTemp.new(iids.heatThrTemp, function (request, self)
                        local value = self:getProp("tar_temp")
                        self.logger:info("Read HeatingThresholdTemperature: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write HeatingThresholdTemperature: " .. value)
                        self:setProp("tar_temp", math.tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end, 16, 30, 1),
                    SwingMode.new(iids.swingMode, function (request, self)
                        local ver_swing = self:getProp("ver_swing")
                        local value
                        if ver_swing == "unsupport" then
                            value = SwingMode.value.Disabled
                        else
                            value = valMapping.ver_swing[ver_swing]
                        end
                        self.logger:info("Read SwingMode: " .. searchKey(SwingMode.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write SwingMode: " .. searchKey(SwingMode.value, value))
                        self:setProp("ver_swing", searchKey(valMapping.ver_swing, value))
                        raiseEvent(request.aid, request.sid, request.cid)
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
