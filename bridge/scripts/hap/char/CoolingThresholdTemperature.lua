local hap = require "hap"

return {
    ---New a ``CoolingThresholdTemperature`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): number
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: number)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "Float", "CoolingThresholdTemperature", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setUnits("Celsius"):setContraints(10, 35, 0.1)
    end
}
