---@meta

---Get manufacturer.
---@return string
local function getManufacturer() end

---Get model.
---@return string
local function getModel() end

---Get serial number.
---@return string
local function getSerialNumber() end

---Get firmware version.
---@return string
local function getFirmwareVersion() end

---Get hardware version.
---@return string
local function getHardwareVersion() end

return {
    getManufacturer = getManufacturer,
    getModel = getModel,
    getSerialNumber = getSerialNumber,
    getFirmwareVersion = getFirmwareVersion,
    getHardwareVersion = getHardwareVersion
}
