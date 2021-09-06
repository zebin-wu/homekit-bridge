return {
    new = function (iid, read)
        return {
            format = "Float",
            iid = iid,
            type = "CurrentTemperature",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true,
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true
                }
            },
            units = "Celsius",
            constraints = {
                minVal = 0,
                maxVal = 100,
                stepVal = 0.1
            },
            cbs = {
                read = read
            }
        }
    end
}
