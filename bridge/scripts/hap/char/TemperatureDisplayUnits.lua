local hap = require "hap"

return {
    value = {
        Celsius = 0,
        Fahrenheit = 1
    },
    ---New a ``TemperatureDisplayUnits`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "TemperatureDisplayUnits", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setContraints(0, 1, 1)
    end
}
