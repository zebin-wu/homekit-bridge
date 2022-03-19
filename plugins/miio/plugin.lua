local hap = require "hap"
local hapUtil = require "hap.util"
local nvs = require "nvs"
local device = require "miio.device"
local traceback = debug.traceback

local M = {}
local logger = log.getLogger("miio.plugin")

---Miio accessory configuration.
---@class MiioAccessoryConf
---
---@field aid integer Accessory Instance ID.
---@field iids table<string, integer> Instance IDs.
---@field addr string Device address.
---@field token string Device token.
---@field sn string Accessory serial number.
---@field name string Accessory name.

---Miio plugin configuration.
---@class MiioPluginConf:PluginConf
---
---@field accessories MiioAccessoryConf[] Accessory configurations.

---Generate accessory via configuration.
---@param conf MiioAccessoryConf Accessory configuration.
---@return HAPAccessory accessory
local function gen(conf)
    local obj = device.create(conf.addr, conf.token)
    local info = obj:getInfo()
    local product = require("miio." .. info.model)
    conf.sn = info.mac:gsub(":", "")
    local handle = nvs.open(conf.sn)
    conf.aid = hapUtil.getBridgedAccessoryIID(handle)
    conf.iids = hapUtil.getInstanceIDs(handle)
    return product.gen(obj, info, conf)
end

---Initialize plugin.
---@param conf MiioPluginConf Plugin configuration.
function M.init(conf)
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

return M
