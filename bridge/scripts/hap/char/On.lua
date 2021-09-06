return {
    new = function (iid, read, write)
        return {
            format = "Bool",
            iid = iid,
            type = "On",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true,
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true
                }
            },
            cbs = {
                read = read,
                write = write
            }
        }
    end
}
