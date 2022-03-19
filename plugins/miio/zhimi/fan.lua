local hap = require "hap"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"
local searchKey = require "util".searchKey
local raiseEvent = hap.raiseEvent

local M = {}

--- Property value -> Characteristic value.
local valMapping = {
    power = {
        on = Active.value.Active,
        off = Active.value.Inactive,
    },
    angle_enable = {
        on = SwingMode.value.Enabled,
        off = SwingMode.value.Disabled
    }
}

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
        name = conf.name or "Zhimi Fan",
        mfg = "zhimi",
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
                        local value = valMapping.power[device:getProp("power")]
                        device.logger:info("Read Active: " .. searchKey(Active.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write Active: " .. searchKey(Active.value, value))
                        device:setProp("power", searchKey(valMapping.power, value))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end),
                    RotationSpeed.new(iids.rotationSpeed, function (request)
                        local value = device:getProp("speed_level")
                        device.logger:info("Read RotationSpeed: " .. value)
                        return value
                    end, function (request, value)
                        device.logger:info("Write RotationSpeed: " .. value)
                        device:setProp("speed_level", math.tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                    end, 1, 100, 1),
                    SwingMode.new(iids.swingMode, function (request)
                        local value = valMapping.angle_enable[device:getProp("angle_enable")]
                        device.logger:info("Read SwingMode: " .. searchKey(SwingMode.value, value))
                        return value
                    end, function (request, value)
                        device.logger:info("Write SwingMode: " .. searchKey(SwingMode.value, value))
                        device:setProp("angle_enable", searchKey(valMapping.angle_enable, value))
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
