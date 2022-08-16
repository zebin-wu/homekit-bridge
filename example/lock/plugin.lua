local hap = require "hap"
local nvs = require "nvs"
local hapUtil = require "hap.util"
local util = require "util"
local LockCurrentState = require "hap.char.LockCurrentState"
local LockTargetState = require "hap.char.LockTargetState"
local LockControlPoint = require "hap.char.LockControlPoint"
local ServiceSignature = require "hap.char.ServiceSignature"
local Version = require "hap.char.Version"
local Name = require "hap.char.Name"
local raiseEvent = hap.raiseEvent

local M = {}

local logger = log.getLogger("lock.plugin")

---Lock accessory configuration.
---@class LockAccessoryConf
---
---@field sn string Serial number.
---@field name string Accessory name.

---Lock plugin configuration.
---@class LockPluginConf:PluginConf
---
---@field accessories LockAccessoryConf[] Accessory configurations.

---Generate accessory via configuration.
---@param conf LockAccessoryConf
---@param handle NVSHandle
---@return HAPAccessory
local function gen(conf, handle)
    local aid = hapUtil.getBridgedAccessoryIID(handle)
    local iids = hapUtil.getInstanceIDs(handle)
    local curState = handle:get("curState") or LockCurrentState.value.Secured
    local tgtState = handle:get("tgtState") or LockTargetState.value.Secured
    local name = conf.name or "Lock"

    return hap.newAccessory(
        aid,
        "BridgedAccessory",
        name,
        "Acme",
        "Lock1,1",
        conf.sn,
        "1",
        "1",
        {
            hap.AccessoryInformationService,
            hap.newService(iids.mechanism, "LockMechanism", true, false, {
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
                            handle:set("tgtState", value)
                            handle:set("curState", value)
                            handle:commit()
                            raiseEvent(request.aid, request.sid, request.cid)
                            raiseEvent(request.aid, request.sid, iids.curState)
                        end
                    end)
            }):linkServices(iids.manage),
            hap.newService(iids.manage, "LockManagement", false, false, {
                ServiceSignature.new(iids.manageSrvSign),
                LockControlPoint.new(iids.manageCtrlPoint, function (request, value) end),
                Version.new(iids.manageVersion, function (request) return "1.0" end)
            }):linkServices(iids.mechanism)
        },
        function (request)
            logger:info("Identify callback is called.")
        end
    )
end

---Initialize plugin.
---@param conf LockPluginConf Plugin configuration.
---@return HAPAccessory[] bridgedAccessories Bridges Accessories.
function M.init(conf)
    local accessories = {}
    for _, accessoryConf in ipairs(conf.accessories) do
        table.insert(accessories, gen(accessoryConf, nvs.open(accessoryConf.sn)))
    end
    return accessories
end

return M
