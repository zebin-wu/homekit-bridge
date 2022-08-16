local hap = require "hap"

return {
    ---New a ``LockControlPoint`` characteristic.
    ---@param iid integer Instance ID.
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: string)
    ---@return HAPCharacteristic characteristic
    new = function (iid, write)
        return hap.newCharacteristic(iid, "TLV8", "LockControlPoint", {
            readable = false,
            writable = true,
            supportsEventNotification = false,
            requiresTimedWrite = true,
        }, nil, write)
    end
}
