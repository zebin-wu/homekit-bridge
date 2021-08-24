local device = require "miio.device"
local util = require "util"

local plugin = {}
local logger = log.getLogger("miio.plugin")
local priv = {
    pending = {},
    devices = {}
}

---Initialize plugin.
---@param conf any Plugin configuration.
---@param report fun(plugin: string, accessory: Accessory) Report accessory to **core**.
---@return boolean status true on success, false on failure.
function plugin.init(conf, report)
    priv.report = report
    logger:info("Initialized.")
    return true
end

---Deinitialize plugin.
function plugin.deinit()
    priv.report = nil
    priv.pending = {}
    priv.devices = {}
    logger:info("Deinitialized.")
end

---Whether the accessory is waiting to be generated.
---@return boolean status
function plugin.isPending()
    return util.isEmptyTable(priv.pending) == false
end

---Report bridged accessory.
---@param obj MiioDevice Device object.
---@param accessory? Accessory Bridged accessory.
local function _report(obj, addr, accessory)
    priv.pending[addr] = nil
    if not accessory then
        priv.devices[addr] = nil
    end
    priv.report("miio", accessory)
end

---Generate accessory via configuration.
function plugin.gen(conf)
    local obj = device.create(function (self, addr)
        if not self.info then
            logger:error("Failed to create device.")
            _report(self, addr)
            return
        end
        -- Get product module, using pcall to catch exception.
        local success, prod = pcall(function (self)
            return require("miio." .. self.info.model)
        end, self)
        if success == false then
            logger:error("Cannot found the product.")
            _report(self, addr)
            return
        end
        local obj = prod.create(self)
        if not obj then
            logger:error("Failed to create the device object.")
            _report(self, addr)
            return
        end
        local accessory = prod.gen(obj)
        if not accessory then
            logger:error("Failed to generate accessory.")
            _report(self, addr)
            return
        end
        _report(self, addr, accessory)
    end, 5000, conf.addr, conf.token, conf.addr)
    if obj then
        priv.pending[conf.addr] = true
        priv.devices[conf.addr] = obj
    end
    return nil
end

return plugin
