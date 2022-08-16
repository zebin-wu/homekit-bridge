local hap = require "hap"

return {
    value = {
        Unsecured = 0,
        Secured = 1
    },
    ---New a ``LockTargetState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "LockTargetState", {
            readable = true,
            writable = true,
            supportsEventNotification = true,
            requiresTimedWrite = true
        }, read, write):setContraints(0, 1, 1)
    end
}
