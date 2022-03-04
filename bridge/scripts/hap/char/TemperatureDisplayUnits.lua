return {
    value = {
        Celsius = 0,
        Fahrenheit = 1
    },
    ---New a ``TemperatureDisplayUnits`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@param write fun(request:HAPCharacteristicWriteRequest, value:any)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return {
            format = "UInt8",
            iid = iid,
            type = "TemperatureDisplayUnits",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            constraints = {
                minVal = 0,
                maxVal = 1,
                stepVal = 1
            },
            cbs = {
                read = read,
                write = write,
            }
        }
    end
}
