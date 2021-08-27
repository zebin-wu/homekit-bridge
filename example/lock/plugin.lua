local plugin = {}

local logger = log.getLogger("lock.plugin")
local isPending = false

function plugin.init()
    logger:info("Initialized.")
    return true
end

function plugin.deinit()
    logger:info("Deinitialized.")
end

function plugin.isPending()
    return isPending
end

local function checkAccessoryConf(conf)
    return true
end

function plugin.gen(conf)
    if checkAccessoryConf(conf) == false then
        return nil
    end

    local hap = require ("hap")
    local util = require ("util")
    local LockCurrentState = require("hap.char.LockCurrentState")
    local LockTargetState = require("hap.char.LockTargetState")
    local LockControlPoint = require("hap.char.LockControlPoint")
    local ServiceSignature = require("hap.char.ServiceSignature")
    local Version = require("hap.char.Version")
    local Name = require("hap.char.Name")

    local context = {
        aid = hap.getNewBridgedAccessoryID(),
        mechanismIID = hap.getNewInstanceID(),
        curStateIID = hap.getNewInstanceID(),
        tgtStateIID = hap.getNewInstanceID(),
        curState = LockCurrentState.value.Secured,
        tgtState = LockTargetState.value.Secured
    }
    return {
        aid = context.aid,
        category = "BridgedAccessory",
        name = conf.name,
        mfg = "Acme",
        model = "Lock1,1",
        sn = conf.sn,
        fwVer = "1",
        hwVer = "1",
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
            {
                iid = context.mechanismIID,
                type = "LockMechanism",
                name = "Lock",
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
                    LockCurrentState.new(context.curStateIID,
                        function (request)
                            logger:info(("Read currentState: %s"):format(
                                util.searchKey(LockCurrentState.value, context.curState)))
                            return context.curState, hap.Error.None
                        end),
                    LockTargetState.new(context.tgtStateIID,
                        function (request)
                            logger:info(("Read targetState: %s"):format(
                                util.searchKey(LockTargetState.value, context.tgtState)))
                            return context.tgtState, hap.Error.None
                        end,
                        function (request, value)
                            local changed = false
                            logger:info(("Write targetState: %s"):format(
                                util.searchKey(LockTargetState.value, value)))
                            if value ~= context.tgtState then
                                context.tgtState = value
                                context.curState = value
                                hap.raiseEvent(context.aid, context.mechanismIID, context.curStateIID)
                                changed = true
                            end
                            return changed, hap.Error.None
                        end)
                }
            },
            {
                iid = hap.getNewInstanceID(),
                type = "LockManagement",
                props = {
                    primaryService = false,
                    hidden = false,
                    ble = {
                        supportsConfiguration = false,
                    }
                },
                chars = {
                    ServiceSignature.new(hap.getNewInstanceID()),
                    LockControlPoint.new(hap.getNewInstanceID(),
                        function (request, value)
                            return false, hap.Error.None
                        end),
                    Version.new(hap.getNewInstanceID(),
                        function (request)
                            return "1.0", hap.Error.None
                        end)
                }
            }
        },
        cbs = {
            identify = function (request)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        }
    }
end

return plugin
