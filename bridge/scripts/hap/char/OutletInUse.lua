return {
    new = function (iid, read)
        return {
            format = "Bool",
            iid = iid,
            type = "On",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = true,
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true
                }
            },
            cbs = {
                read = read
            }
        }
    end
}
