return {
    ---New a ``Version`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "String",
            iid = iid,
            type = "Version",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = false,
            },
            constraints = { maxLen = 64 },
            cbs = {
                read = read
            }
        }
    end
}
