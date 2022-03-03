return {
    ---New a ``WaterLevel`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest): any, HapError
    ---@return HapCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "Float",
            iid = iid,
            type = "WaterLevel",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            units = "Percentage",
            constraints = {
                minVal = 0,
                maxVal = 100,
                stepVal = 1
            },
            cbs = {
                read = read
            }
        }
    end
}
