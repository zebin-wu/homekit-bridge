return {
    value = {
        Unsecured = 0,
        Secured = 1
    },
    ---New a ``LockTargetState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest): any
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any)
    ---@return HapCharacteristic characteristic
    new = function (iid, read, write)
        return {
            format = "UInt8",
            iid = iid,
            type = "LockTargetState",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true,
                requiresTimedWrite = true
            },
            constraints = {
                minVal = 0,
                maxVal = 1,
                stepVal = 1
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
