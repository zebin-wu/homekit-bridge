local version = {
    format = "String",
    permissions = {
        read = true,
        write = false,
        notify= false
    }
}

function version.new(iid, read)
    local self = version
    return {
        format = self.format,
        iid = iid,
        type = "Version",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
        },
        constraints = { maxLen = 64 },
        cbs = {
            read = read
        }
    }
end

return version
