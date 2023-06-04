local hap = require "hap"
local On = require "hap.char.On"
local raiseEvent = hap.raiseEvent

local M = {}

---@class PlugDevice:MiioDevice
---
---@field getOn fun(self: MiioDevice): boolean
---@field setOn fun(self: MiioDevice, value: boolean)

---Create a plug.
---@param device PlugDevice Device object.
---@param conf MiioAccessoryConf Device configuration.
---@return HAPAccessory accessory HomeKit Accessory.
function M.gen(device, conf)
    local iids = conf.iids

    return hap.newAccessory(
        conf.aid,
        "BridgedAccessory",
        conf.name or "Chuangmi Plug",
        "chuangmi",
        conf.model,
        conf.sn,
        conf.fw_ver,
        conf.hw_ver,
        {
            hap.AccessoryInformationService,
            hap.newService(iids.outlet, "Outlet", true, false, {
                On.new(iids.on, function (request)
                    return device:getOn()
                end, function (request, value)
                    device:setOn(value)
                    raiseEvent(request.aid, request.sid, request.cid)
                end)
            })
        },
        function (request)
            device.logger:info("Identify callback is called.")
        end
    )
end

return M
