local hap = require "hap"
local On = require "hap.char.On"

local plug = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@param on string The property name of ``On``.
---@return HapAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf, on)
    ---@class PlugIIDs:table Plug Instance ID table.
    local iids = {
        acc = hap.getNewBridgedAccessoryID(),
        outlet = hap.getNewInstanceID(),
        on = hap.getNewInstanceID()
    }

    device:regProps({ on },
    ---@param self MiioDevice Device Object.
    ---@param names string[] Property Names.
    ---@param iids PlugIIDs Plug Instance ID table.
    function (self, names, iids)
        hap.raiseEvent(iids.acc, iids.outlet, iids.on)
    end, iids)

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Plug",
        mfg = "chuangmi",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = iids.outlet,
                type = "Outlet",
                name = "Outlet",
                props = {
                    primaryService = true,
                    hidden = false,
                    ble = {
                        supportsConfiguration = false,
                    }
                },
                chars = {
                    On.new(iids.on, function (request, self)
                        local value = self:getOn()
                        self.logger:info(("Read On: %s"):format(value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info(("Write On: %s"):format(value))
                        self:setOn(value)
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

return plug
