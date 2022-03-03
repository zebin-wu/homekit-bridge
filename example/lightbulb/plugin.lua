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
    local iids = {
        acc = hap.getNewBridgedAccessoryID(),
        lightBlub = hap.getNewInstanceID(),
        srvSign = hap.getNewInstanceID(),
        name = hap.getNewInstanceID(),
        on = hap.getNewInstanceID()
    }
    local lightBulbOn = false
    local name = conf.name or "Light Bulb"
    return {
        aid = iids.acc,
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
                                hap.raiseEvent(request.aid, request.sid, request.cid)
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
        hap.addBridgedAccessory(gen(accessoryConf))
    end
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
