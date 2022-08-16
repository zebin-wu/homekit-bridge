local hap = require "hap"

return {
    value = {
        Inactive = 0,
        Idle = 1,
        Heating = 2,
        Cooling = 3
    },
    ---New a ``CurrentHeaterCoolerState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "UInt8", "CurrentHeaterCoolerState", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setContraints(0, 3, 1)
    end
}
