local hap = require "hap"
local char = require "hap.char"

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

local function lightBulbOnCharacteristic(iid)
    return {
        format = "Bool",
        iid = iid,
        type = "On",
        props = {
            readable = true,
            writable = true,
            supportsEventNotification = true,
            hidden = false,
            requiresTimedWrite = false,
            supportsAuthorizationData = false,
            ip = { controlPoint = false, supportsWriteResponse = false },
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true,
                readableWithoutSecurity = false,
                writableWithoutSecurity = false
            }
        },
        cbs = {
            read = function (request, context)
                logger:info(("Read lightBulbOn: %s"):format(context.lightBulbOn))
                return context.lightBulbOn, hap.Error.None
            end,
            write = function (request, value, context)
                local changed = false
                logger:info(("Write lightBulbOn: %s"):format(value))
                if value ~= context.lightBulbOn then
                    context.lightBulbOn = value
                    changed = true
                end
                return changed, hap.Error.None
            end
        }
    }
end

function lightbulb.gen(conf)
    if checkAccessoryConf(conf) == false then
        return nil
    end
    local state = {
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
                    char.newServiceSignatureCharacteristic(hap.getNewInstanceID()),
                    char.newNameCharacteristic(hap.getNewInstanceID()),
                    lightBulbOnCharacteristic(hap.getNewInstanceID())
                }
            }
        },
        cbs = {
            identify = function (request, context)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        },
        context = state,
    }
end

return lightbulb
