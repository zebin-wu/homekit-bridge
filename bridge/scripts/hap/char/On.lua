return {
    ---New a ``On`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@param write fun(request:HAPCharacteristicWriteRequest, value:any)
    ---@return HAPCharacteristic characteristic
    new = function (iid, read, write)
        return {
            format = "Bool",
            iid = iid,
            type = "On",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
