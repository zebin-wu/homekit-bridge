local hap = require "hap"
local util = require "util"
local LockCurrentState = require "hap.char.LockCurrentState"
local LockTargetState = require "hap.char.LockTargetState"
local LockControlPoint = require "hap.char.LockControlPoint"
local ServiceSignature = require "hap.char.ServiceSignature"
local Version = require "hap.char.Version"
local Name = require "hap.char.Name"

local plugin = {}

local logger = log.getLogger("lock.plugin")

---Lock accessory configuration.
---@class LockAccessoryConf
---
---@field sn integer Serial number.
---@field name string Accessory name.

---Lock plugin configuration.
---@class LockPluginConf:PluginConf
---
---@field accessories LockAccessoryConf[] Accessory configurations.

---Generate accessory via configuration.
---@param conf LockAccessoryConf
---@return HapAccessory
local function gen(conf)
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
                        function (request, context)
                            logger:info(("Read currentState: %s"):format(
                                util.searchKey(LockCurrentState.value, context.curState)))
                            return context.curState, hap.Error.None
                        end),
                    LockTargetState.new(context.tgtStateIID,
                        function (request, context)
                            logger:info(("Read targetState: %s"):format(
                                util.searchKey(LockTargetState.value, context.tgtState)))
                            return context.tgtState, hap.Error.None
                        end,
                        function (request, value, context)
                            logger:info(("Write targetState: %s"):format(
                                util.searchKey(LockTargetState.value, value)))
                            if value ~= context.tgtState then
                                context.tgtState = value
                                context.curState = value
                                hap.raiseEvent(context.aid, context.mechanismIID, context.curStateIID)
                                hap.raiseEvent(request.accessory.aid,
                                    request.service.iid, request.characteristic.iid)
                            end
                            return hap.Error.None
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
                        function (request, value, context)
                            return hap.Error.None
                        end),
                    Version.new(hap.getNewInstanceID(),
                        function (request, context)
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
        },
        context = context
    }
end

---Initialize plugin.
---@param conf LockPluginConf Plugin configuration.
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
