local lockCurrentState = {
    format = "UInt8",
    permissions = {
        read = true,
        write = false,
        notify= true
    },
    minVal = 0,
    maxVal = 3,
    stepVal = 1,
    value = {
        Unsecured = 0,
        Secured = 1,
        Jammed = 2,
        Unknown = 3
    }
}

function lockCurrentState.new(iid, read)
    local self = lockCurrentState
    return {
        format = self.format,
        iid = iid,
        type = "LockCurrentState",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "None",
        constraints = {
            minVal = self.minVal,
            maxVal = self.maxVal,
            stepVal = self.stepVal,
        },
        cbs = {
            read = read
        }
    }
end

return lockCurrentState
