local lockControlPoint = {
    format = "TLV8",
    permissions = {
        read = false,
        write = true,
        notify= false
    }
}

function lockControlPoint.new(iid, write)
    local self = lockControlPoint
    return {
        format = self.format,
        iid = iid,
        type = "LockControlPoint",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
            requiresTimedWrite = true,
        },
        cbs = {
            write = write
        }
    }
end

return lockControlPoint
