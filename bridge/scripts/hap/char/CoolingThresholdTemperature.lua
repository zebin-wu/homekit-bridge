return {
    ---New a ``CoolingThresholdTemperature`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest): any, HapError
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any): HapError
    ---@param minVal? number Minimum value.
    ---@param maxVal? number Maximum value.
    ---@param stepVal? number Step value.
    ---@return HapCharacteristic characteristic
    new = function (iid, read, write, minVal, maxVal, stepVal)
        return {
            format = "Float",
            iid = iid,
            type = "CoolingThresholdTemperature",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            units = "Celsius",
            constraints = {
                minVal = minVal or 10,
                maxVal = maxVal or 35,
                stepVal = stepVal or 0.1
            },
            cbs = {
                read = read,
                write = write,
            }
        }
    end
}
