local hap = require "hap"

return {
    ---New a ``RelativeHumidityDehumidifierThreshold`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): number
    ---@param write fun(request: HAPCharacteristicWriteRequest, value: number)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "Float", "RelativeHumidityDehumidifierThreshold", {
            readable = true,
            writable = true,
            supportsEventNotification = true
        }, read, write):setUnits("Percentage"):setContraints(0, 100, 1)
    end
}
