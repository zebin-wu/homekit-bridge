return {
    ---New a ``On`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest): any, HapError
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any): HapError
    ---@return HapCharacteristic characteristic
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
