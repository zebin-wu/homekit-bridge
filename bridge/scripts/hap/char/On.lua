local on = {
    format = "Bool",
    permissions = {
        read = true,
        write = true,
        notify= true
    }
}

function on.new(iid, read, write)
    local self = on
    return {
        format = self.format,
        iid = iid,
        type = "On",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
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

return on
