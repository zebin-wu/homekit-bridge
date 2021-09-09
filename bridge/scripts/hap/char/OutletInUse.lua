return {
    new = function (iid, read)
        return {
            format = "Bool",
            iid = iid,
            type = "OutletInUse",
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
