local version = {}

function version.new(iid, read)
    return {
        format = "String",
        iid = iid,
        type = "Version",
        props = {
            readable = true,
            writable = false,
            supportsEventNotification = false,
        },
        constraints = { maxLen = 64 },
        cbs = {
            read = read
        }
    }
end

return version
