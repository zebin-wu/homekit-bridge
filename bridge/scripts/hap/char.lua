local hap = require "hap"

local char = {}

---``ENUM`` Lock Current State.
char.LockCurrentState = {
    Unsecured = 0,
    Secured = 1,
    Jammed = 2,
    Unknown = 3
}

---``ENUM`` Lock Target State.
char.LockTargetState = {
    Unsecured = 0,
    Secured = 1
}

function char.newServiceSignatureCharacteristic(iid)
    return {
        format = "Data",
        iid = iid,
        type = "ServiceSignature",
        props = {
            readable = true,
            writable = false,
            supportsEventNotification = false,
            hidden = false,
            requiresTimedWrite = false,
            supportsAuthorizationData = false,
            ip = { controlPoint = true },
            ble = {
                supportsBroadcastNotification = false,
                supportsDisconnectedNotification = false,
                readableWithoutSecurity = false,
                writableWithoutSecurity = false
            }
        },
        constraints = { maxLen = 2097152 },
        cbs = {
            read = function (request, context)
                return "", hap.Error.None
            end
        }
    }
end

function char.newNameCharacteristic(iid)
    return {
        format = "String",
        iid = iid,
        type = "Name",
        props = {
            readable = true,
            writable = false,
            supportsEventNotification = false,
            hidden = false,
            requiresTimedWrite = false,
            supportsAuthorizationData = false,
            ip = { controlPoint = false, supportsWriteResponse = false },
            ble = {
                supportsBroadcastNotification = false,
                supportsDisconnectedNotification = false,
                readableWithoutSecurity = false,
                writableWithoutSecurity = false
            }
        },
        constraints = { maxLen = 64 },
        cbs = {
            read = function (request, context)
                return request.service.name, hap.Error.None
            end
        }
    }
end

return char
