return {
    ---New a ``Name`` characteristic.
    ---@param iid integer Instance ID.
    ---@param name string Service name.
    ---@return HapCharacteristic characteristic
    new = function (iid, name)
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
                read = function (request)
                    return name, hap.Error.None
                end
            }
        }
    end
}
