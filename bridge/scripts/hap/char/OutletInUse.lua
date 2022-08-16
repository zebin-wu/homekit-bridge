local hap = require "hap"

return {
    ---New a ``OutletInUse`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): boolean
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "Bool", "OutletInUse", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read)
    end
}
