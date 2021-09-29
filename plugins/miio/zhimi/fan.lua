local hap = require "hap"
local util = require "util"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"

local fan = {}

---Create a fan.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function fan.gen(device, info, conf)
    local iids = {
        acc = hap.getNewBridgedAccessoryID()
    }

    for i, v in ipairs({
        "fan", "active", "rotationSpeed"
    }) do
        iids[v] = hap.getNewInstanceID()
    end

    device:registerProps({ "power", "speed_level" }, function (self, name, iids)
        if name == "power" then
            hap.raiseEvent(iids.acc, iids.fan, iids.active)
        end
    end, iids)

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Fan",
        mfg = "zhimi",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = iids.fan,
                type = "Fan",
                name = "Fan",
                props = {
                    primaryService = true,
                    hidden = false,
                    ble = {
                        supportsConfiguration = false,
                    }
                },
                chars = {
                    Active.new(iids.active, function (request, self)
                        local value
                        if self:getProp("power") == "on" then
                            value = Active.value.Active
                        else
                            value = Active.value.Inactive
                        end
                        self.logger:info("Read Active: " .. util.searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write Active: " .. util.searchKey(Active.value, value))
                        local power
                        if value == Active.value.Active then
                            power = "on"
                        else
                            power = "off"
                        end
                        self:setProp("power", power)
                        return hap.Error.None
                    end),
                    RotationSpeed.new(iids.rotationSpeed, function (request, self)
                        local value = self:getProp("speed_level")
                        self.logger:info("Read RotationSpeed: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write RotationSpeed: " .. value)
                        self:setProp("speed_level", math.tointeger(value))
                        return hap.Error.None
                    end, 1, 100, 1),

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

return fan
