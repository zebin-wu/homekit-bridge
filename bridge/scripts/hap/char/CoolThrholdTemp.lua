local coolThrholdTemp = {}

function coolThrholdTemp.new(iid, read, write, minVal, maxVal, stepVal)
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
            minVal = minVal or 10,
            maxVal = maxVal or 35,
            stepVal = stepVal or 0.1
        },
        cbs = {
            read = read,
            write = write,
        }
    }
end

return coolThrholdTemp
