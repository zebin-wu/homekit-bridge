local hap = require "hap"

return {
    ---New a ``WaterLevel`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): number
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return hap.newCharacteristic(iid, "Float", "WaterLevel", {
            readable = true,
            writable = false,
            supportsEventNotification = true
        }, read):setUnits("Percentage"):setContraints(0, 100, 1)
    end
}
