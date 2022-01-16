local device = require "miio.device"
local util = require "util"

local plugin = {}
local logger = log.getLogger("miio.plugin")

---@class MiioDeviceConf:AccessoryConf Miio device configuration.
---
---@field addr string Device address.
---@field token string Device token.

---Initialize plugin.
---@param conf any Plugin configuration.
---@return boolean status true on success, false on failure.
function plugin.init(conf)
    logger:info("Initialized.")
end

---Handle HAP server state.
---@param state HapServerState
function plugin.handleState(state)
    logger:info("HAP server state: " .. state .. ".")
end

---Generate accessory via configuration.
---@param conf MiioDeviceConf Device configuration.
---@return HapAccessory accessory
function plugin.gen(conf)
    local obj = device.create(conf.addr, util.hex2bin(conf.token))
    local info = obj:getInfo()
    return require("miio." .. info.model).gen(obj, info, conf)
end

return plugin
