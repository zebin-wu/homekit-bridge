local hap = require "hap"

return {
    value = {
        Unsecured = 0,
        Secured = 1,
        Jammed = 2,
        Unknown = 3
    },
    ---New a ``LockCurrentState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "UInt8", "LockCurrentState", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setContraints(0, 3, 1)
    end
}
