local plugin = {}

local logger = log.getLogger("lightbulb.plugin")
local isPending = false

function plugin.init()
    logger:info("Initialized.")
    return true
end

function plugin.isPending()
    return isPending
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

local function checkAccessoryConf(conf)
    return true
end

function plugin.gen(conf)
    if checkAccessoryConf(conf) == false then
        return nil
    end

    local hap = require("hap")
    local ServiceSignature = require("hap.char.ServiceSignature")
    local Name = require("hap.char.Name")
    local On = require("hap.char.On")

    local context = {
        lightBulbOn = false,
    }
    return {
        aid = conf.aid,
        category = "BridgedAccessory",
        name = conf.name,
        mfg = "Acme",
        model = "LightBulb1,1",
        sn = conf.sn,
        fwVer = "1",
        hwVer = "1",
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = hap.getNewInstanceID(),
                type = "LightBulb",
                name = "Light Bulb",
                props = {
                    primaryService = true,
                    hidden = false,
                    ble = {
                        supportsConfiguration = false,
                    }
                },
                chars = {
                    ServiceSignature.new(hap.getNewInstanceID()),
                    Name.new(hap.getNewInstanceID()),
                    On.new(hap.getNewInstanceID(),
                        function (request, context)
                            logger:info(("Read lightBulbOn: %s"):format(context.lightBulbOn))
                            return context.lightBulbOn, hap.Error.None
                        end,
                        function (request, value, context)
                            logger:info(("Write lightBulbOn: %s"):format(value))
                            if value ~= context.lightBulbOn then
                                context.lightBulbOn = value
                                hap.raiseEvent(request.accessory.aid,
                                    request.service.iid, request.characteristic.iid)
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

return plugin
