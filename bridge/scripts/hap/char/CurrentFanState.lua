local hap = require "hap"

return {
    value = {
        Inactive = 0,
        Idle = 1,
        BlowingAir = 2,
    },
    ---New a ``CurrentFanState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "UInt8", "CurrentFanState", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setContraints(0, 2, 1)
    end
}
