---@meta

---Starts the accessory server.
---@param accessory table
---@param bridgedaccessories? table
---@param configurationChanged? boolean
---@return boolean
local function start(accessory, bridgedaccessories, configurationChanged) end

---Stops the accessory server.
---@return boolean
local function stop() end

---Accessory category.
local AccessoryCategory = {
    BridgedAccessory = 0,
    Other = 1,
    Bridges = 2,
    Fans = 3,
    GarageDoorOpeners = 4,
    Lighting = 5,
    Locks = 6,
}

---Error type.
local Error = {
    None = 0,
    Unknown = 1,
    InvalidState = 2,
    InvalidData = 3,
    OutOfResources = 4,
    NotAuthorized = 5,
    Busy = 6,
}

---HomeKit Accessory Information service.
---@type lightuserdata
local AccessoryInformationService

---HAP Protocol Information service.
---@type lightuserdata
local HapProtocolInformationService

---Pairing service.
---@type lightuserdata
local PairingService

return {
    start = start,
    stop = stop,
    AccessoryCategory = AccessoryCategory,
    Error = Error,
    AccessoryInformationService = AccessoryInformationService,
    HapProtocolInformationService = HapProtocolInformationService,
    PairingService = PairingService,
}
