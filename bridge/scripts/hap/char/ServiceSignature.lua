local serviceSignature = {
    format = "Data",
    permissions = {
        read = true,
        write = false,
        notify= false
    }
}

function serviceSignature:new(iid)
    local hap = require("hap")
    return {
        format = self.format,
        iid = iid,
        type = "ServiceSignature",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify,
            ip = { controlPoint = true }
        },
        constraints = { maxLen = 2097152 },
        cbs = {
            read = function (request, context)
                return "", hap.Error.None
            end
        }
    }
end

return serviceSignature
