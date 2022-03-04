return {
    ---New a ``OutletInUse`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HAPCharacteristicReadRequest): any
    ---@return HAPCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "Bool",
            iid = iid,
            type = "OutletInUse",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            cbs = {
                read = read
            }
        }
    end
}
