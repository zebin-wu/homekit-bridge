local coolThrholdTemp = {}

function coolThrholdTemp.new(iid, read, write)
    return {
        format = "Float",
        iid = iid,
        type = "CoolingThresholdTemperature",
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
            minVal = 10,
            maxVal = 35,
            stepVal = 0.1
        },
        cbs = {
            read = read,
            write = write,
        }
    }
end

return coolThrholdTemp
