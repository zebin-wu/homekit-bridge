local hap = require "hap"
local device = require "miio.device"
local util = require "util"
local traceback = debug.traceback

local plugin = {}
local logger = log.getLogger("miio.plugin")

---Miio accessory configuration.
---@class MiioAccessoryConf
---
---@field aid integer Accessory Instance ID.
---@field addr string Device address.
---@field token string Device token.
---@field name string Accessory name.

---Miio plugin configuration.
---@class MiioPluginConf:PluginConf
---
---@field accessories MiioAccessoryConf[] Accessory configurations.

---Generate accessory via configuration.
---@param conf MiioAccessoryConf Accessory configuration.
---@return HapAccessory accessory
local function gen(conf)
    local obj = device.create(conf.addr, util.hex2bin(conf.token))
    local info = obj:getInfo()
    local product = require("miio." .. info.model)
    conf.aid = hap.getNewBridgedAccessoryID()
    return product.gen(obj, info, conf)
end

---Initialize plugin.
---@param conf MiioPluginConf Plugin configuration.
function plugin.init(conf)
    logger:info("Initialized.")

    for _, accessoryConf in ipairs(conf.accessories) do
        local success, result = xpcall(gen, traceback, accessoryConf)
        if success == false then
            logger:error(result)
        else
            hap.addBridgedAccessory(result)
        end
    end
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
