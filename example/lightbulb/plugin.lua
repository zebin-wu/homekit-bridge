local hap = require "hap"
local ServiceSignature = require "hap.char.ServiceSignature"
local Name = require "hap.char.Name"
local On = require "hap.char.On"

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
---@return HapAccessory
local function gen(conf)
    local context = {
        lightBulbOn = false,
    }
    local name = conf.name or "Light Bulb"
    return {
        aid = hap.getNewBridgedAccessoryID(),
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
                iid = hap.getNewInstanceID(),
                type = "LightBulb",
                props = {
                    primaryService = true,
                    hidden = false
                },
                chars = {
                    ServiceSignature.new(hap.getNewInstanceID()),
                    Name.new(hap.getNewInstanceID(), name),
                    On.new(hap.getNewInstanceID(),
                        function (request, context)
                            logger:info(("Read lightBulbOn: %s"):format(context.lightBulbOn))
                            return context.lightBulbOn, hap.Error.None
                        end,
                        function (request, value, context)
                            logger:info(("Write lightBulbOn: %s"):format(value))
                            if value ~= context.lightBulbOn then
                                context.lightBulbOn = value
                                hap.raiseEvent(request.aid, request.sid, request.cid)
                            end
                            return hap.Error.None
                        end)
                }
            }
        },
        cbs = {
            identify = function (request, context)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        },
        context = context
    }
end

---Initialize plugin.
---@param conf LightBulbPluginConf Plugin configuration.
function plugin.init(conf)
    logger:info("Initialized.")

    for _, accessoryConf in ipairs(conf.accessories) do
        hap.addBridgedAccessory(gen(accessoryConf))
    end
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
