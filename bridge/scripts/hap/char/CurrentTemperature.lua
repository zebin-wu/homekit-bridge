return {
    ---New a ``CurrentTemperature`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "Float",
            iid = iid,
            type = "CurrentTemperature",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            units = "Celsius",
            constraints = {
                minVal = 0,
                maxVal = 100,
                stepVal = 0.1
            },
            cbs = {
                read = read
            }
        }
    end
}
