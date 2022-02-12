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
---@param mapping table<string, MiotIID> Property name -> MIOT instance ID mapping.
---@return HapAccessory accessory HomeKit Accessory.
function fan.gen(device, info, conf, mapping)
    ---@class DmakerFanIIDs:table Dmaker Fan Instance ID table.
    local iids = {
        acc = conf.aid,
        fan = hap.getNewInstanceID(),
        active = hap.getNewInstanceID(),
        rotationSpeed = hap.getNewInstanceID(),
        swingMode = hap.getNewInstanceID(),
    }

    device:setMapping(mapping)

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
                    RotationSpeed.new(iids.rotationSpeed, function (request, self)
                        local value = self:getProp("fanSpeed")
                        self.logger:info("Read RotationSpeed: " .. value)
                        return tonumber(value), hap.Error.None
                    end, function (request, value, self)
                        self.logger:info("Write RotationSpeed: " .. value)
                        self:setProp("fanSpeed", math.tointeger(value))
                        raiseEvent(request.aid, request.sid, request.cid)
                        return hap.Error.None
                    end, 1, 100, 1),
                    SwingMode.new(iids.swingMode, function (request, self)
                        local swingModeVal = SwingMode.value
                        local value
                        if self:getProp("swingMode") then
                            value = swingModeVal.Enabled
                        else
                            value = swingModeVal.Disabled
                        end
                        self.logger:info("Read SwingMode: " .. searchKey(swingModeVal, value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        local swingModeVal = SwingMode.value
                        self.logger:info("Write SwingMode: " .. searchKey(swingModeVal, value))
                        self:setProp("swingMode", value == swingModeVal.Enabled)
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

return fan
