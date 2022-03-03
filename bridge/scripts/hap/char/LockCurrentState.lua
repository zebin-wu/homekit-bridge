return {
    value = {
        Unsecured = 0,
        Secured = 1,
        Jammed = 2,
        Unknown = 3
    },
    ---New a ``LockCurrentState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest): any, HapError
    ---@return HapCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "UInt8",
            iid = iid,
            type = "LockCurrentState",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            constraints = {
                minVal = 0,
                maxVal = 3,
                stepVal = 1,
            },
            cbs = {
                read = read
            }
        }
    end
}
