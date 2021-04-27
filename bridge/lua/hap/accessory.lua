local accessory = {}

local iid = 0

---New a accessory.
---@param name string The display name of the accessory.
---@param category AccessoryCategory Category information for the accessory.
---@param mfg string The manufacturer of the accessory.
---@param model string The model name of the accessory.
---@param sn string The serial number of the accessory.
---@param fver string The firmware version of the accessory.
---@param hver string The hardware version of the accessory.
---@param services? Service[] Array of provided services.
---@return Accessory accessory
function accessory.new(name, category, mfg, model, sn, fver, hver, services)
    return {
        aid = accessory.iid(),
        category = category,
        name = name,
        manufacturer = mfg,
        model = model,
        sn = sn,
        firmwareVersion = fver,
        hardwareVersion = hver,
        services = services or {},
    }
end

---Add callbacks to accessory.
---@param a Accessory
---@param identify fun(request: AccessoryIdentifyRequest):integer The callback used to invoke the identify routine.
function accessory.addCallbacks(a, identify)
    a.callbacks.identify = identify
end

---Add service to accessory.
---@param a Accessory
---@param s Service
function accessory.addService(a, s)
    table.insert(a.services, s)
end

---Get a new accessory instance ID.
---@return integer aid Accessory instance ID
function accessory.iid()
    iid = iid + 1
    return iid
end

return accessory
