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
    for i, o in ipairs(priv.devices) do
        o:destroy()
    end
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
local function _report(obj, accessory)
    priv.pending[obj.info.netif.localIp] = nil
    if not accessory then
        obj:destroy()
    end
    priv.report("miio", accessory)
end

---Generate accessory via configuration.
function plugin.gen(conf)
    assert(device.create(function (self)
        if not self.info then
            logger:error("Failed to create device.")
            _report(self)
            return
        end
        -- Get product module, using pcall to catch exception.
        local success, prod = pcall(function (self)
            return require("miio." .. self.info.model)
        end, self)
        if success == false then
            logger:error("Cannot found the product.")
            _report(self)
            return
        end
        local obj = prod.create(self)
        if not obj then
            logger:error("Failed to create the device object.")
            _report(self)
            return
        end
        local accessory = prod.gen(obj)
        if not accessory then
            logger:error("Failed to generate accessory.")
            _report(self)
            return
        end
        table.insert(priv.devices, obj)
        _report(self, accessory)
    end, 5, conf.addr, conf.token))
    priv.pending[conf.addr] = true
    return nil
end

return plugin
