return {
    value = {
        Inactive = 0,
        Idle = 1,
        Humidifying = 2,
        Dehumidifying = 3
    },
    ---New a ``CurrentHumidifierDehumidifierState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "UInt8",
            iid = iid,
            type = "CurrentHumidifierDehumidifierState",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            constraints = {
                minVal = 0,
                maxVal = 3,
                stepVal = 1
            },
            cbs = {
                read = read
            }
        }
    end
}
