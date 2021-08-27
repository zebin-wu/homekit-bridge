local active = {
    value = {
        Inactive = 0,
        Active = 1
    }
}

function active.new(iid, read, write)
    return {
        format = "UInt8",
        iid = iid,
        type = "Active",
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
        units = "None",
        constraints = {
            minVal = 0,
            maxVal = 1,
            stepVal = 1
        },
        cbs = {
            read = read,
            write = write
        }
    }
end

return active
