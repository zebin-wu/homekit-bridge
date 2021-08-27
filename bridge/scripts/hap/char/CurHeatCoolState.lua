local curHeaterCoolState = {
    value = {
        Inactive = 0,
        Idle = 1,
        Heating = 2,
        Cooling = 3
    }
}

function curHeaterCoolState.new(iid, read)
    return {
        format = "UInt8",
        iid = iid,
        type = "CurrentHeaterCoolerState",
        props = {
            readable = true,
            writable = false,
            supportsEventNotification = true,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "None",
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

return curHeaterCoolState
