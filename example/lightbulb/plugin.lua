local hap = require "hap"
local hapUtil = require "hap.util"
local nvs = require "nvs"
local ServiceSignature = require "hap.char.ServiceSignature"
local Name = require "hap.char.Name"
local On = require "hap.char.On"
local raiseEvent = hap.raiseEvent

local plugin = {}

local logger = log.getLogger("lightbulb.plugin")

---LightBulb accessory configuration.
---@class LightBulbAccessoryConf
---
---@field sn integer Serial number.
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

    return {
        aid = aid,
        category = "BridgedAccessory",
        name = name,
        mfg = "Acme",
        model = "LightBulb1,1",
        sn = conf.sn,
        fwVer = "1",
        hwVer = "1",
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.lightBlub,
                type = "LightBulb",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    ServiceSignature.new(iids.srvSign),
                    Name.new(iids.name, name),
                    On.new(iids.on, function (request)
                            logger:info(("Read lightBulbOn: %s"):format(lightBulbOn))
                            return lightBulbOn
                        end,
                        function (request, value)
                            logger:info(("Write lightBulbOn: %s"):format(value))
                            if value ~= lightBulbOn then
                                lightBulbOn = value
                                handle:set("on", lightBulbOn)
                                handle:commit()
                                raiseEvent(request.aid, request.sid, request.cid)
                            end
                        end)
                }
            }
        },
        cbs = {
            identify = function (request)
                logger:info("Identify callback is called.")
            end
        }
    }
end

---Initialize plugin.
---@param conf LightBulbPluginConf Plugin configuration.
function plugin.init(conf)
    logger:info("Initialized.")

    for _, accessoryConf in ipairs(conf.accessories) do
        hap.addBridgedAccessory(gen(accessoryConf, nvs.open(accessoryConf.sn)))
    end
end

---Handle HAP accessory server state.
---@param state HAPAccessoryServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
