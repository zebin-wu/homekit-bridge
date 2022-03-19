---@meta

---@class chiplib
local M = {}

---@alias ChipInfoType
---|'"mfg"'     # Manufacturer
---|'"model"'   # Model
---|'"sn"'      # Serial number
---|'"hwver"'   # Hardware version

---Get chip inforamtion.
---@param type ChipInfoType
---@return string
---@nodiscard
function M.getInfo(type) end

return M
