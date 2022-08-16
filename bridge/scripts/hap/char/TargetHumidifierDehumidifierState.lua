local hap = require "hap"

return {
    value = {
        HumidifierOrDehumidifier= 0,
        Humidifier = 1,
        Dehumidifier = 2
    },
    ---New a ``TargetHumidifierDehumidifierState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write? fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "TargetHumidifierDehumidifierState", {
            readable = true,
            writable = write and true or false,
            supportsEventNotification = true
        }, read, write):setContraints(0, 2, 1)
    end
}
