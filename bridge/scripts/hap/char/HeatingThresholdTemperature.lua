return {
    ---New a ``HeatingThresholdTemperature`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest, context?:any): any, HapError
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any, context?:any): HapError
    ---@param minVal? number Minimum value.
    ---@param maxVal? number Maximum value.
    ---@param stepVal? number Step value.
    ---@return HapCharacteristic characteristic
    new = function (iid, read, write, minVal, maxVal, stepVal)
        return {
            format = "Float",
            iid = iid,
            type = "HeatingThresholdTemperature",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            units = "Celsius",
            constraints = {
                minVal = minVal or 0,
                maxVal = maxVal or 25,
                stepVal = stepVal or 0.1
            },
            cbs = {
                read = read,
                write = write,
            }
        }
    end
}
