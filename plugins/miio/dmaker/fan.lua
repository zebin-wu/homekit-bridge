local hap = require "hap"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent

local fan = {}

---Create a fan.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function fan.gen(device, info, conf)
    local iids = {
        acc = conf.aid,
        fan = hap.getNewInstanceID(),
        active = hap.getNewInstanceID(),
        rotationSpeed = hap.getNewInstanceID(),
        swingMode = hap.getNewInstanceID(),
    }

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Dmaker Fan",
        mfg = "dmaker",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.fan,
                type = "Fan",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    Active.new(iids.active, function (request)
                        local activeVal = Active.value
                        local value
                        if device:getProp("power") then
                            value = activeVal.Active
                        else
                            value = activeVal.Inactive
                        end
                        device.logger:info("Read Active: " .. searchKey(activeVal, value))
                        return value, hap.Error.None
                    end, function (request, value)
                        local activeVal = Active.value
                        device.logger:info("Write Active: " .. searchKey(activeVal, value))
                        device:setProp("power", value == activeVal.Active)
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end),
                    RotationSpeed.new(iids.rotationSpeed, function (request)
                        local value = device:getProp("fanSpeed")
                        device.logger:info("Read RotationSpeed: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value)
                        device.logger:info("Write RotationSpeed: " .. value)
                        device:setProp("fanSpeed", math.tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end, 1, 100, 1),
                    SwingMode.new(iids.swingMode, function (request)
                        local swingModeVal = SwingMode.value
                        local value
                        if device:getProp("swingMode") then
                            value = swingModeVal.Enabled
                        else
                            value = swingModeVal.Disabled
                        end
                        device.logger:info("Read SwingMode: " .. searchKey(swingModeVal, value))
                        return value, hap.Error.None
                    end, function (request, value)
                        local swingModeVal = SwingMode.value
                        device.logger:info("Write SwingMode: " .. searchKey(swingModeVal, value))
                        device:setProp("swingMode", value == swingModeVal.Enabled)
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end)
                }
            }
        },
        cbs = {
            identify = function (request)
                device.logger:info("Identify callback is called.")
                return hap.Error.None
            end
        }
    }
end

return fan
