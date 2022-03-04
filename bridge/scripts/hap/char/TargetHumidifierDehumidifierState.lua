return {
    value = {
        HumidifierOrDehumidifier= 0,
        Humidifier = 1,
        Dehumidifier = 2
    },
    ---New a ``TargetHumidifierDehumidifierState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@param write fun(request:HAPCharacteristicWriteRequest, value:any)
    ---@param minVal? number Minimum value.
    ---@param maxVal? number Maximum value.
    ---@param stepVal? number Step value.
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write, minVal, maxVal, stepVal)
        return {
            format = "UInt8",
            iid = iid,
            type = "TargetHumidifierDehumidifierState",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            constraints = {
                minVal = minVal or 0,
                maxVal = maxVal or 2,
                stepVal = stepVal or 1
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
