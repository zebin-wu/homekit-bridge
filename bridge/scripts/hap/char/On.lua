local hap = require "hap"

return {
    ---New a ``On`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): boolean
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: boolean)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "Bool", "On", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write)
    end
}
