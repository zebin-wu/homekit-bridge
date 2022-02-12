return {
    value = {
        Inactive = 0,
        Idle = 1,
        Heating = 2,
        Cooling = 3
    },
    ---New a ``CurrentHeaterCoolerState`` characteristic.
    ---@param iid integer Instance ID.
    ---@param read fun(request:HapCharacteristicReadRequest, context?:any): any, HapError
    ---@return HapCharacteristic characteristic
    new = function (iid, read)
        return {
            format = "UInt8",
            iid = iid,
            type = "CurrentHeaterCoolerState",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true
            },
            constraints = {
                minVal = 0,
                maxVal = 3,
                stepVal = 1
            },
            cbs = {
                read = read
            }
        }
    end
}
