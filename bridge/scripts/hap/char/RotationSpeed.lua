return {
    new = function (iid, read, write)
        return {
            format = "Float",
            iid = iid,
            type = "RotationSpeed",
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
            units = "Percentage",
            constraints = {
                minVal = 0,
                maxVal = 100,
                stepVal = 1
            },
            cbs = {
                read = read,
                write = write,
            }
        }
    end
}
