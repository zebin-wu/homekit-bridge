local hap = require "hap"

return {
    value = {
        Inactive = 0,
        Idle = 1,
        Humidifying = 2,
        Dehumidifying = 3
    },
    ---New a ``CurrentHumidifierDehumidifierState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "UInt8", "CurrentHumidifierDehumidifierState", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setContraints(0, 3, 1)
    end
}
