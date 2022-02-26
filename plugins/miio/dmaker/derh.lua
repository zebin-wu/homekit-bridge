local hap = require "hap"
local Active = require "hap.char.Active"
local CurrentState = require "hap.char.CurrentHumidifierDehumidifierState"
local TargetState = require "hap.char.TargetHumidifierDehumidifierState"
local CurrentHumidity = require "hap.char.CurrentRelativeHumidity"
local TargetHumidity = require "hap.char.RelativeHumidityDehumidifierThreshold"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent
local tointeger = math.tointeger

local derh = {}

---Create a dehumidifier.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function derh.gen(device, info, conf)
    ---@class DmakerFanIIDs:table Dmaker Fan Instance ID table.
    local iids = {
        acc = conf.aid,
        derh = hap.getNewInstanceID(),
        active = hap.getNewInstanceID(),
        curState = hap.getNewInstanceID(),
        tgtState = hap.getNewInstanceID(),
        curHumidity = hap.getNewInstanceID(),
        tgtHumidity = hap.getNewInstanceID()
    }
    device.iids = iids

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Dmaker Dehumidifier",
        mfg = "dmaker",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.derh,
                type = "HumidifierDehumidifier",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    Active.new(iids.active, function (request, self)
                        local activeVal = Active.value
                        local value
                        if self:getProp("power") then
                            value = activeVal.Active
                        else
                            value = activeVal.Inactive
                        end
                        self.logger:info("Read Active: " .. searchKey(activeVal, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        local activeVal = Active.value
                        self.logger:info("Write Active: " .. searchKey(activeVal, value))
                        self:setProp("power", value == activeVal.Active)
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end),
                    CurrentState.new(iids.curState, function (request, self)
                        local value
                        if self:getProp("power") then
                            value = CurrentState.value.Dehumidifying
                        else
                            value = CurrentState.value.Inactive
                        end
                        self.logger:info("Read CurrentHumidifierDehumidifierState: " .. searchKey(CurrentState.value, value))
                        return value, hap.Error.None
                    end),
                    TargetState.new(iids.tgtState, function (request, self)
                        local value = TargetState.value.Dehumidifier
                        self.logger:info("Read TargetHumidifierDehumidifierState: Dehumidifier")
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write TargetHumidifierDehumidifierState: " .. searchKey(TargetState.value, value))
                        return hap.Error.None
                    end, TargetState.value.Dehumidifier, TargetState.value.Dehumidifier, 0),
                    CurrentHumidity.new(iids.curHumidity, function (request, self)
                        local value =  self:getProp("curHumidity")
                        self.logger:info("Read CurrentRelativeHumidity: " .. tointeger(value))
                        return value, hap.Error.None
                    end),
                    TargetHumidity.new(iids.tgtHumidity, function (request, self)
                        local value = self:getProp("tgtHumidity")
                        self.logger:info("Read RelativeHumidityDehumidifierThreshold: " .. tointeger(value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write RelativeHumidityDehumidifierThreshold: " .. tointeger(value))
                        self:setProp("tgtHumidity", tointeger(value))
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

return derh
