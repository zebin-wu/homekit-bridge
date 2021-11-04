local hap = require "hap"
local util = require "util"
local Active = require "hap.char.Active"
local RotationSpeed = require "hap.char.RotationSpeed"
local SwingMode = require "hap.char.SwingMode"

local fan = {}

--- Property -> Characteristic.
local mapping = {
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
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory HomeKit Accessory.
function fan.gen(device, info, conf)
    ---@class FanIIDs:table Fan Instance ID table.
    local iids = {
        acc = hap.getNewBridgedAccessoryID(),
        fan = hap.getNewInstanceID(),
        active = hap.getNewInstanceID(),
        rotationSpeed = hap.getNewInstanceID(),
        swingMode = hap.getNewInstanceID(),
    }

    ---Update callback called when property is updated.
    ---@param self MiioDevice Device Object.
    ---@param name string Property Name.
    ---@param iids FanIIDs Fan Instance ID table.
    local function _update(self, name, iids)
        if name == "power" then
            hap.raiseEvent(iids.acc, iids.fan, iids.active)
        elseif name == "speed_level" then
            hap.raiseEvent(iids.acc, iids.fan, iids.rotationSpeed)
        elseif name == "angle_enable" then
            hap.raiseEvent(iids.acc, iids.fan, iids.swingMode)
        end
    end

    device:registerProps({
        "power", "speed_level", "angle_enable"
    }, _update, iids)

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
                        local value = mapping.power[self:getProp("power")]
                        self.logger:info("Read Active: " .. util.searchKey(Active.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write Active: " .. util.searchKey(Active.value, value))
                        self:setProp("power", util.searchKey(mapping.power, value))
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
                    SwingMode.new(iids.swingMode, function (request, self)
                        local value = mapping.angle_enable[self:getProp("angle_enable")]
                        self.logger:info("Read SwingMode: " .. util.searchKey(SwingMode.value, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write SwingMode: " .. util.searchKey(SwingMode.value, value))
                        self:setProp("angle_enable", util.searchKey(mapping.angle_enable, value))
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

return fan
