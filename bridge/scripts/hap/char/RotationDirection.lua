local hap = require "hap"

return {
    value = {
        Clockwise = 0,
        CounterClockwise = 1
    },
    ---New a ``RotationDirection`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "Int", "RotationDirection", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setContraints(0, 1, 1)
    end
}
