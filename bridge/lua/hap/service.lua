local hap = require("hap")

local service = {}

---New a service.
---@param name string The name of the service.
---@param type ServiceType The type of the service.
---@param primary boolean The service is the primary service on the accessory.
---@param hidden boolean The service should be hidden from the user.
---@param chars? Characteristic[] Array of contained characteristics.
---@return Service service
function service.new(name, type, primary, hidden, chars)
    return {
        iid = hap.getInstanceID(),
        name = name,
        type = type,
        props = {
            primaryService = primary,
            hidden = hidden,
            ble = {
                supportsConfiguration = false,
            }
        },
        chars = chars or {}
    }
end

---Add characteristic to the service.
---@param s Service
---@param c Characteristic
function service.addCharacteristic(s, c)
    table.insert(s.chars, c)
end

return service
