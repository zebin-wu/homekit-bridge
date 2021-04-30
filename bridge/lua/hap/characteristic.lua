local characteristic = {}

---New a characteristic.
---@param format CharacteristicFormat Format.
---@param type CharacteristicType The type of the characteristic.
---@param mfgDesc string Description of the characteristic provided by the manufacturer of the accessory.
---@param props CharacteristicProperties Characteristic properties.
---@return Characteristic
function characteristic.new(format, type, mfgDesc, props)
    return {
        iid = hap.getNewInstanceID(),
        format = format,
        type = type,
        mfgDesc = mfgDesc,
        props = props,
    }
end

---Set units to the characteristic.
---@param c Characteristic
---@param units CharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
function characteristic.setUnits(c, units)
    c.units = units
end

---Set constraints to the characteristic.
---@param c Characteristic
---@param constraints StringCharacteristiConstraints|NumberCharacteristiConstraints|UInt8CharacteristiConstraints Value constraints.
function characteristic.setConstraints(c, constraints)
    c.constraints = constraints
end

---Add callbacks to the characteristic.
---@param c Characteristic
---@param read function The callback used to handle read requests.
---@param write function The callback used to handle write requests.
---@param sub function The callback used to handle subscribe requests.
---@param unsub function The callback used to handle unsubscribe requests.
function characteristic.setCallbacks(c, read, write, sub, unsub)
    c.callbacks = {
        read = read,
        write = write,
        sub = sub,
        unsub = unsub
    }
end

return characteristic
