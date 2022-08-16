local hap = require "hap"
local hapUtil = require "hap.util"
local nvs = require "nvs"
local ServiceSignature = require "hap.char.ServiceSignature"
local Name = require "hap.char.Name"
local On = require "hap.char.On"
local raiseEvent = hap.raiseEvent

local M = {}

local logger = log.getLogger("lightbulb.plugin")

---LightBulb accessory configuration.
---@class LightBulbAccessoryConf
---
---@field sn string Serial number.
---@field name string Accessory name.

---LightBulb plugin configuration.
---@class LightBulbPluginConf:PluginConf
---
---@field accessories LightBulbAccessoryConf[] Accessory configurations.

---Generate accessory via configuration.
---@param conf LightBulbAccessoryConf
---@param handle NVSHandle
---@return HAPAccessory
local function gen(conf, handle)
    local aid = hapUtil.getBridgedAccessoryIID(handle)
    local iids = hapUtil.getInstanceIDs(handle)
    local lightBulbOn = handle:get("on") or false
    local name = conf.name or "Light Bulb"

    return hap.newAccessory(
        aid,
        "BridgedAccessory",
        name,
        "Acme",
        "LightBulb1,1",
        conf.sn,
        "1",
        "1",
        {
            hap.AccessoryInformationService,
            hap.newService(iids.lightBlub, "LightBulb", true, false, {
                ServiceSignature.new(iids.srvSign),
                Name.new(iids.name, name),
                On.new(iids.on, function (request)
                    return lightBulbOn
                end,
                function (request, value)
                    if value ~= lightBulbOn then
                        lightBulbOn = value
                        handle:set("on", lightBulbOn)
                        handle:commit()
                        raiseEvent(request.aid, request.sid, request.cid)
                    end
                end)
            })
        },
        function (request)
            logger:info("Identify callback is called.")
        end
    )
end

---Initialize plugin.
---@param conf LightBulbPluginConf Plugin configuration.
---@return HAPAccessory[] bridgedAccessories Bridges Accessories.
function M.init(conf)
    local accessories = {}
    for _, accessoryConf in ipairs(conf.accessories) do
        table.insert(accessories, gen(accessoryConf, nvs.open(accessoryConf.sn)))
    end
    return accessories
end

return M
