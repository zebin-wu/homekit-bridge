local hap = require "hap"

return {
    ---New a ``Version`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): string
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "String", "Version", {
            readable = true,
            writable = false,
            supportsEventNotification = false,
        }, read):setContraints(64)
    end
}
