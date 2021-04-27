---@meta

---@class pallib
pal = {}

local board

---Get manufacturer.
---@return string
function board.getManufacturer() end

---Get model.
---@return string
function board.getModel() end

---Get serial number.
---@return string
function board.getSerialNumber() end

---Get firmware version.
---@return string
function board.getFirmwareVersion() end

---Get hardware version.
---@return string
function board.getHardwareVersion() end

pal.board = board

return pal
