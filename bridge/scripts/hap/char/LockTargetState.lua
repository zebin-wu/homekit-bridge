local lockTargetState = {
    format = "UInt8",
    permissions = {
        read = true,
        write = true,
        notify= true
    },
    minVal = 0,
    maxVal = 1,
    stepVal = 1,
    value = {
        Unsecured = 0,
        Secured = 1
    }
}

function lockTargetState:new(iid, read, write)
    return {
        format = self.format,
        iid = iid,
        type = "LockTargetState",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
            requiresTimedWrite = true,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "None",
        constraints = {
            minVal = self.minVal,
            maxVal = self.maxVal,
            stepVal = self.stepVal
        },
        cbs = {
            read = read,
            write = write
        }
    }
end

return lockTargetState
