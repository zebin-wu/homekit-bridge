return {
    ---New a ``ServiceSignature`` characteristic.
    ---@param iid integer Instance ID.
    ---@return HapCharacteristic characteristic
    new = function (iid)
        local hap = require("hap")
        return {
            format = "Data",
            iid = iid,
            type = "ServiceSignature",
            props = {
                readable = true,
                writable = false,
                supportsEventNotification = false,
                ip = { controlPoint = true }
            },
            constraints = { maxLen = 2097152 },
            cbs = {
                read = function (request)
                    return "", hap.Error.None
                end
            }
        }
    end
}
