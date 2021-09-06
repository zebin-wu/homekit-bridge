return {
    new = function (iid, read, write, minVal, maxVal, stepVal)
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
                minVal = minVal or 0,
                maxVal = maxVal or 25,
                stepVal = stepVal or 0.1
            },
            cbs = {
                read = read,
                write = write,
            }
        }
    end
}
