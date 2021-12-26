---@class chiplib
local chip = {}

---@alias ChipInfoType
---|'"mfg"'     # Manufacturer
---|'"model"'   # Model
---|'"sn"'      # Serial number
---|'"hwver"'   # Hardware version

---Get chip inforamtion.
---@param type ChipInfoType
---@return string
function chip.getInfo(type) end

return chip
