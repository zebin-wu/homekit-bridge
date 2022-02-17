local hap = require "hap"
local On = require "hap.char.On"
local raiseEvent = hap.raiseEvent

local plug = {}

---Create a plug.
---@param device MiioDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@param on string The property name of ``On``.
---@return HapAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf, on)
    ---@class PlugIIDs:table Plug Instance ID table.
    local iids = {
        acc = conf.aid,
        outlet = hap.getNewInstanceID(),
        on = hap.getNewInstanceID()
    }

    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = conf.name or "Chuangmi Plug",
        mfg = "chuangmi",
        model = info.model,
        sn = info.mac,
        fwVer = info.fw_ver,
        hwVer = info.hw_ver,
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.outlet,
                type = "Outlet",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    On.new(iids.on, function (request, self)
                        local value = self:getOn()
                        self.logger:info(("Read On: %s"):format(value))
                        return value, hap.Error.None
                    end, function (request, value, self)
                        self.logger:info(("Write On: %s"):format(value))
                        self:setOn(value)
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

return plug
