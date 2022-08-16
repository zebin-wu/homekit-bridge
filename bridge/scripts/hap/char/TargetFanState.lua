local hap = require "hap"

return {
    value = {
        Manual= 0,
        Auto = 1
    },
    ---New a ``TargetFanState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "TargetFanState", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setContraints(0, 1, 1)
    end
}
