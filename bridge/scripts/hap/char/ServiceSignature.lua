local hap = require("hap")

return {
    ---New a ``ServiceSignature`` characteristic.
    ---@param iid integer Instance ID.
    ---@return HAPCharacteristic characteristic
    new = function (iid)
        return hap.newCharacteristic(iid, "Data", "ServiceSignature", {
            readable = true,
            writable = false,
            supportsEventNotification = false,
            ip = { controlPoint = true }
        }, function (request)
            return ""
        end):setContraints(2097152)
    end
}
