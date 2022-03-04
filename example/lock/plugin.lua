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
---@return HAPAccessory
local function gen(conf)
    local iids = {
        acc = hap.getNewBridgedAccessoryID(),
        mechanism = hap.getNewInstanceID(),
        mechanismSrvSign = hap.getNewInstanceID(),
        mechanismName = hap.getNewInstanceID(),
        curState = hap.getNewInstanceID(),
        tgtState = hap.getNewInstanceID(),
        manage = hap.getNewInstanceID(),
        manageSrvSign = hap.getNewInstanceID(),
        manageCtrlPoint = hap.getNewInstanceID(),
        manageVersion = hap.getNewInstanceID()
    }
    local curState = LockCurrentState.value.Secured
    local tgtState = LockTargetState.value.Secured
    local name = conf.name or "Lock"
    return {
        aid = iids.acc,
        category = "BridgedAccessory",
        name = name,
        mfg = "Acme",
        model = "Lock1,1",
        sn = conf.sn,
        fwVer = "1",
        hwVer = "1",
        services = {
            hap.AccessoryInformationService,
            {
                iid = iids.mechanism,
                type = "LockMechanism",
                props = {
                    primaryService = true,
                    hidden = false
                },
                linkedServices = { iids.manage },
                chars = {
                    ServiceSignature.new(iids.mechanismSrvSign),
                    Name.new(iids.mechanismName, name),
                    LockCurrentState.new(iids.curState,
                        function (request)
                            logger:info(("Read currentState: %s"):format(
                                util.searchKey(LockCurrentState.value, curState)))
                            return curState
                        end),
                    LockTargetState.new(iids.tgtState,
                        function (request)
                            logger:info(("Read targetState: %s"):format(
                                util.searchKey(LockTargetState.value, tgtState)))
                            return tgtState
                        end,
                        function (request, value)
                            logger:info(("Write targetState: %s"):format(
                                util.searchKey(LockTargetState.value, value)))
                            if value ~= tgtState then
                                tgtState = value
                                curState = value
                                hap.raiseEvent(request.aid, request.sid, request.cid)
                                hap.raiseEvent(request.aid, request.sid, iids.curState)
                            end
                        end)
                }
            },
            {
                iid = iids.manage,
                type = "LockManagement",
                props = {
                    primaryService = false,
                    hidden = false
                },
                linkedServices = { iids.mechanism },
                chars = {
                    ServiceSignature.new(iids.manageSrvSign),
                    LockControlPoint.new(iids.manageCtrlPoint, function (request, value) end),
                    Version.new(iids.manageVersion, function (request) return "1.0" end)
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
---@param conf LockPluginConf Plugin configuration.
function plugin.init(conf)
    logger:info("Initialized.")

    for _, accessoryConf in ipairs(conf.accessories) do
        hap.addBridgedAccessory(gen(accessoryConf))
    end
end

---Handle HAP server state.
---@param state HAPServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
