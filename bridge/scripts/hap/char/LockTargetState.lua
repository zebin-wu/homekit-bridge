local lockTargetState = {
    value = {
        Unsecured = 0,
        Secured = 1
    }
}

function lockTargetState.new(iid, read, write)
    return {
        format = "UInt8",
        iid = iid,
        type = "LockTargetState",
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

return lockTargetState
