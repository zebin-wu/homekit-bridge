local hap = require "hap"

return {
    value = {
        HeatOrCool= 0,
        Heat = 1,
        Cool = 2
    },
    ---New a ``TargetHeaterCoolerState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request: HAPCharacteristicReadRequest): integer
    ---@param write? fun(request: HAPCharacteristicWriteRequest, value: integer)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return hap.newCharacteristic(iid, "UInt8", "TargetHeaterCoolerState", {
            readable = true,
            writable = write and true or false,
            supportsEventNotification = true
        }, read, write):setContraints(0, 2, 1)
    end
}
