return {
    ---New a ``OutletInUse`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest, context?:any): any, HapError
    ---@return HapCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "Bool",
            iid = iid,
            type = "OutletInUse",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true,
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true
                }
            },
            cbs = {
                read = read
            }
        }
    end
}
