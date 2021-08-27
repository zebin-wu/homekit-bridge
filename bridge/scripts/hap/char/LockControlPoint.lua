local lockControlPoint = {}

function lockControlPoint.new(iid, write)
    return {
        format = "TLV8",
        iid = iid,
        type = "LockControlPoint",
        props = {
            readable = false,
            writable = true,
            supportsEventNotification = false,
            requiresTimedWrite = true,
        },
        cbs = {
            write = write
        }
    }
end

return lockControlPoint
