return {
    value = {
        HeatOrCool= 0,
        Heat = 1,
        Cool = 2
    },
    ---New a ``TargetHeaterCoolerState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest, context?:any): any, HapError
    ---@param write fun(request:HapCharacteristicWriteRequest, value:any, context?:any): HapError
    ---@return HapCharacteristic characteristic
    new = function (iid, read, write)
        return {
            format = "UInt8",
            iid = iid,
            type = "TargetHeaterCoolerState",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true,
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true
                }
            },
            constraints = {
                minVal = 0,
                maxVal = 2,
                stepVal = 1
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
