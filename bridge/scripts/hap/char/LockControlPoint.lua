return {
    ---New a ``LockControlPoint`` characteristic.
    ---@param iid integer Instance ID.
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any): HapError
    ---@return HapCharacteristic characteristic
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
