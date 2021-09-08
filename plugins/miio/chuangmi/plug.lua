local hap = require "hap"
local On = require "hap.char.On"
local OutletInUse = require "hap.char.OutletInUse"

local plug = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioDeviceConf Device configuration.
---@param on string The property name of ``On``.
---@return HapAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf, on)
    local iids = {
        acc = hap.getNewBridgedAccessoryID()
    }

    for i, v in ipairs({
        "outlet", "on", "inuse"
    }) do
        iids[v] = hap.getNewInstanceID()
    end

    device:registerProps({ on }, function (self, name, iids)
        if name == on then
            hap.raiseEvent(iids.acc, iids.outlet, iids.on)
            hap.raiseEvent(iids.acc, iids.outlet, iids.inuse)
        end
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
                    end),
                    OutletInUse.new(iids.inuse, function (request, self)
                        local value = self:getOn()
                        self.logger:info(("Read OutletInUse: %s"):format(value))
                        return value, hap.Error.None
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
