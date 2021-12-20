local device = require "miio.device"
local protocol = require "miio.protocol"
local util = require "util"

local plugin = {}
local logger = log.getLogger("miio.plugin")
local priv = {
    pending = {},
    devices = {}
}

---@class MiioDeviceConf:table Miio device configuration.
---
---@field name string Accessory name.
---@field addr string Device address.
---@field token string Device token.

---Initialize plugin.
---@param conf any Plugin configuration.
---@param report fun(plugin: string, accessory: HapAccessory) Report accessory to **core**.
---@return boolean status true on success, false on failure.
function plugin.init(conf, report)
    protocol.init()
    priv.report = report
    logger:info("Initialized.")
    return true
end

---Deinitialize plugin.
function plugin.deinit()
    priv.report = nil
    priv.pending = {}
    priv.devices = {}
    protocol.deinit()
    logger:info("Deinitialized.")
end

---Whether the accessory is waiting to be generated.
---@return boolean status
function plugin.isPending()
    return util.isEmptyTable(priv.pending) == false
end

---Report bridged accessory.
---@param obj MiioDevice Device object.
---@param accessory? HapAccessory Bridged accessory.
local function _report(obj, addr, accessory)
    priv.pending[addr] = nil
    if not accessory then
        priv.devices[addr] = nil
    end
    priv.report("miio", accessory)
end

---Generate accessory via configuration.
---@param conf MiioDeviceConf Device configuration.
---@return nil
function plugin.gen(conf)
    local obj = device.create(function (self, info, conf)
        if not info then
            logger:error("Failed to create device.")
            _report(self, conf.addr)
            return
        end
        -- Get product module, using pcall to catch exception.
        local success, result = pcall(require, "miio." .. info.model)
        if success == false then
            logger:error("Cannot found the product.")
            logger:error(result)
            _report(self, conf.addr)
            return
        end
        local accessory = result.gen(self, info, conf)
        if not accessory then
            logger:error("Failed to generate accessory.")
            _report(self, conf.addr)
            return
        end
        _report(self, conf.addr, accessory)
    end, conf.addr, util.hex2bin(conf.token), conf)
    if obj then
        priv.pending[conf.addr] = true
        priv.devices[conf.addr] = obj
    end
    return nil
end

return plugin
