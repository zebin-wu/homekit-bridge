local hap = require "hap"

return {
    ---New a ``CurrentRelativeHumidity`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): number
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "Float", "CurrentRelativeHumidity", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setUnits("Percentage"):setContraints(0, 100, 1)
    end
}
