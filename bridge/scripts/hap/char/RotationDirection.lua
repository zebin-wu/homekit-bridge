return {
    value = {
        Clockwise = 0,
        CounterClockwise = 1
    },
    ---New a ``RotationDirection`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@param write fun(request:HAPCharacteristicWriteRequest, value:any)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return {
            format = "Int",
            iid = iid,
            type = "RotationDirection",
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
