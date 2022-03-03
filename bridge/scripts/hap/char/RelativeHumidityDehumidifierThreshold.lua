return {
    ---New a ``RelativeHumidityDehumidifierThreshold`` characteristic.
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
            type = "RelativeHumidityDehumidifierThreshold",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            units = "Percentage",
            constraints = {
                minVal = minVal or 0,
                maxVal = maxVal or 100,
                stepVal = stepVal or 1
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
