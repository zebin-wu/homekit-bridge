local hap = require "hap"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"
local tointeger = math.tointeger
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent

local M = {}

---Create a fan.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, info, conf)
    local iids = conf.iids

    return {
        aid = conf.aid,
        category = "BridgedAccessory",
        name = conf.name or "Dmaker Fan",
        mfg = "dmaker",
        model = info.model,
        sn = conf.sn,
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
                        return value
                    end, function (request, value)
                        local activeVal = Active.value
                        device.logger:info("Write Active: " .. searchKey(activeVal, value))
                        device:s_power(value == activeVal.Active)
                        raiseEvent(request.aid, request.sid, request.cid)
                    end),
                    RotationSpeed.new(iids.rotationSpeed, function (request)
                        local value = device:getProp("speed")
                        device.logger:info("Read RotationSpeed: " .. value)
                        return value
                    end, function (request, value)
                        device.logger:info("Write RotationSpeed: " .. value)
                        device:s_speed(tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end, 1, 100, 1),
                    SwingMode.new(iids.swingMode, function (request)
                        local swingModeVal = SwingMode.value
                        local value
                        if device:getProp("roll_enable") then
                            value = swingModeVal.Enabled
                        else
                            value = swingModeVal.Disabled
                        end
                        device.logger:info("Read SwingMode: " .. searchKey(swingModeVal, value))
                        return value
                    end, function (request, value)
                        local swingModeVal = SwingMode.value
                        device.logger:info("Write SwingMode: " .. searchKey(swingModeVal, value))
                        device:s_roll(value == swingModeVal.Enabled)
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
