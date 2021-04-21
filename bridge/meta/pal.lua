---@meta

---Get manufacturer.
---@return string
local function board_getManufacturer() end

---Get model.
---@return string
local function board_getModel() end

---Get serial number.
---@return string
local function board_getSerialNumber() end

---Get firmware version.
---@return string
local function board_getFirmwareVersion() end

---Get hardware version.
---@return string
local function board_getHardwareVersion() end

return {
    board = {
        getManufacturer = board_getManufacturer,
        getModel = board_getModel,
        getSerialNumber = board_getSerialNumber,
        getFirmwareVersion = board_getFirmwareVersion,
        getHardwareVersion = board_getHardwareVersion,
    }
}
