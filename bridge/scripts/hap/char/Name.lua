return {
    new = function (iid)
        local hap = require("hap")
        return {
            format = "String",
            iid = iid,
            type = "Name",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = false
            },
            constraints = { maxLen = 64 },
            cbs = {
                read = function (request, context)
                    return request.service.name, hap.Error.None
                end
            }
        }
    end
}
