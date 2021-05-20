local name = {
    format = "String",
    permissions = {
        read = true,
        write = false,
        notify = false
    }
}

function name:new(iid)
    local hap = require("hap")
    return {
        format = self.format,
        iid = iid,
        type = "Name",
        props = {
            readable = self.permissions.read,
            writable = self.permissions.write,
            supportsEventNotification = self.permissions.notify
        },
        constraints = { maxLen = 64 },
        cbs = {
            read = function (request, context)
                return request.service.name, hap.Error.None
            end
        }
    }
end

return name
