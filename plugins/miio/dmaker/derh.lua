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
---@return HAPAccessory accessory HomeKit Accessory.
function derh.gen(device, info, conf)
    local iids = conf.iids

    return {
        aid = conf.aid,
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
                    Active.new(iids.active, function (request)
                        local value
                        if device:getProp("power") then
                            value = Active.value.Active
                        else
                            value = Active.value.Inactive
                        end
                        device.logger:info("Read Active: " .. searchKey(Active.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write Active: " .. searchKey(Active.value, value))
                        device:setProp("power", value == Active.value.Active)
                        raiseEvent(request.aid, request.sid, request.cid)
                        raiseEvent(request.aid, request.sid, iids.curState)
                    end),
                    CurrentState.new(iids.curState, function (request)
                        local value
                        if device:getProp("power") then
                            value = CurrentState.value.Dehumidifying
                        else
                            value = CurrentState.value.Inactive
                        end
                        device.logger:info("Read CurrentHumidifierDehumidifierState: " .. searchKey(CurrentState.value, value))
                        return value
                    end),
                    TargetState.new(iids.tgtState, function (request)
                        local value = TargetState.value.Dehumidifier
                        device.logger:info("Read TargetHumidifierDehumidifierState: Dehumidifier")
                        return value
                    end, function (request, value)
                        device.logger:info("Write TargetHumidifierDehumidifierState: " .. searchKey(TargetState.value, value))
                    end, TargetState.value.Dehumidifier, TargetState.value.Dehumidifier, 0),
                    CurrentHumidity.new(iids.curHumidity, function (request)
                        local value =  device:getProp("curHumidity")
                        device.logger:info("Read CurrentRelativeHumidity: " .. tointeger(value))
                        return value
                    end),
                    TargetHumidity.new(iids.tgtHumidity, function (request)
                        local value = device:getProp("tgtHumidity")
                        device.logger:info("Read RelativeHumidityDehumidifierThreshold: " .. tointeger(value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write RelativeHumidityDehumidifierThreshold: " .. tointeger(value))
                        device:setProp("tgtHumidity", tointeger(value))
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

return derh
