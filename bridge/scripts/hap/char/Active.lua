local hap = require "hap"

return {
    value = {
        Inactive = 0,
        Active = 1
    },
    ---New a ``Active`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "Active", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setContraints(0, 1, 1)
    end
}
