local heatThrholdTemp = {}

function heatThrholdTemp.new(iid, read, write)
    return {
        format = "Float",
        iid = iid,
        type = "HeatingThresholdTemperature",
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
            maxVal = 25,
            stepVal = 0.1
        },
        cbs = {
            read = read,
            write = write,
        }
    }
end

return heatThrholdTemp
