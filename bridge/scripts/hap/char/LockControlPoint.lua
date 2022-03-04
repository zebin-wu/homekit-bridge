return {
    ---New a ``LockControlPoint`` characteristic.
    ---@param iid integer Instance ID.
    ---@param write fun(request:HAPCharacteristicWriteRequest, value:any)
    ---@return HAPCharacteristic characteristic
    new = function (iid, write)
        return {
            format = "TLV8",
            iid = iid,
            type = "LockControlPoint",
            props = {
                readable = false,
                writable = true,
                supportsEventNotification = false,
                requiresTimedWrite = true,
            },
            cbs = {
                write = write
            }
        }
    end
}
