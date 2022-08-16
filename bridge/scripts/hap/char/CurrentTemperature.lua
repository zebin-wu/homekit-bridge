local hap = require "hap"

return {
    ---New a ``CurrentTemperature`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): number
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "Float", "CurrentTemperature", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setUnits("Celsius"):setContraints(0, 100, 0.1)
    end
}
