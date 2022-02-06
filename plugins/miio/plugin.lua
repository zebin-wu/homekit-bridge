local hap = require "hap"
local device = require "miio.device"
local util = require "util"
local traceback = debug.traceback

local plugin = {}
local logger = log.getLogger("miio.plugin")

---@class MiioDeviceConf:AccessoryConf Miio device configuration.
---
---@field aid integer Accessory Instance ID.
---@field addr string Device address.
---@field token string Device token.

---Generate accessory via configuration.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory
local function gen(conf)
    local obj = device.create(conf.addr, util.hex2bin(conf.token))
    local info = obj:getInfo()
    local product = require("miio." .. info.model)
    conf.aid = hap.getNewBridgedAccessoryID()
    return product.gen(obj, info, conf)
end

---Initialize plugin.
---@param conf PluginConf Plugin configuration.
---@return HapAccessory[]
function plugin.init(conf)
    logger:info("Initialized.")

    local accessories = {}
    for _, accessoryConf in ipairs(conf.accessories) do
        local success, result = xpcall(gen, traceback, accessoryConf)
        if success == false then
            logger:error(result)
        else
            table.insert(accessories, result)
        end
    end
    return accessories
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

return plugin
