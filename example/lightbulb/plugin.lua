local lightbulb = {}

local logger = log.getLogger("lightbulb")
local inited = false

function lightbulb.isInited()
    return inited
end

function lightbulb.init()
    inited = true
    logger:info("Initialized.")
    return true
end

function lightbulb.deinit()
    inited = false
    logger:info("Deinitialized.")
end

local function checkAccessoryConf(conf)
    return true
end

function lightbulb.gen(conf)
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
        aid = hap.getNewBridgedAccessoryID(),
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
                        function (request)
                            logger:info(("Read lightBulbOn: %s"):format(context.lightBulbOn))
                            return context.lightBulbOn, hap.Error.None
                        end,
                        function (request, value)
                            local changed = false
                            logger:info(("Write lightBulbOn: %s"):format(value))
                            if value ~= context.lightBulbOn then
                                context.lightBulbOn = value
                                changed = true
                            end
                            return changed, hap.Error.None
                        end)
                }
            }
        },
        cbs = {
            identify = function (request, context)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        }
    }
end

return lightbulb
