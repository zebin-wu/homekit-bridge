local lockCurrentState = {
    value = {
        Unsecured = 0,
        Secured = 1,
        Jammed = 2,
        Unknown = 3
    }
}

function lockCurrentState.new(iid, read)
    return {
        format = "UInt8",
        iid = iid,
        type = "LockCurrentState",
        props = {
            readable = true,
            writable = false,
            supportsEventNotification = true,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "None",
        constraints = {
            minVal = 0,
            maxVal = 3,
            stepVal = 1,
        },
        cbs = {
            read = read
        }
    }
end

return lockCurrentState
