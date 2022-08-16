local hap = require "hap"

return {
    ---New a ``Name`` characteristic.
    ---@param iid integer Instance ID.
    ---@param name string Service name.
    ---@return HAPCharacteristic characteristic
    new = function (iid, name)
        return hap.newCharacteristic(iid, "String", "Name", {
            readable = true,
            writable = false,
            supportsEventNotification = false
        }, function (request)
            return name
        end):setContraints(64)
    end
}
