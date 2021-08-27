local tempDisplayUnits = {
    value = {
        Celsius = 0,
        Fahrenheit = 1
    }
}

function tempDisplayUnits.new(iid, read, write)
    return {
        format = "UInt8",
        iid = iid,
        type = "TemperatureDisplayUnits",
        props = {
            readable = true,
            writable = true,
            supportsEventNotification = true,
            requiresTimedWrite = true,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "Celsius",
        constraints = {
            minVal = 0,
            maxVal = 1,
            stepVal = 1
        },
        cbs = {
            read = read,
            write = write,
        }
    }
end

return tempDisplayUnits
