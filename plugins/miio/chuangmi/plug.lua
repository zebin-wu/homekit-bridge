local hap = require "hap"
local On = require "hap.char.On"
local raiseEvent = hap.raiseEvent

local plug = {}

---@class PlugDevice:MiioDevice
---
---@field getOn fun(): boolean
---@field setOn fun(value: boolean)

---Create a plug.
---@param device PlugDevice Device object.
---@param info MiioDeviceInfo Device inforamtion.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function plug.gen(device, info, conf)
    local iids = conf.iids

    return {
        aid = conf.aid,
        category = "BridgedAccessory",
        name = conf.name or "Chuangmi Plug",
        mfg = "chuangmi",
        model = info.model,
        sn = conf.sn,
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
                    On.new(iids.on, function (request)
                        local value = device:getOn()
                        device.logger:info(("Read On: %s"):format(value))
                        return value
                    end, function (request, value)
                        device.logger:info(("Write On: %s"):format(value))
                        device:setOn(value)
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

return plug
