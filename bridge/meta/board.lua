---@class boardlib
local board = {}

---@alias BoardInfoType
---|'"mfg"'     # Manufacturer
---|'"model"'   # Model
---|'"sn"'      # Serial number
---|'"fwver"'   # Firmware version
---|'"hwver"'   # Hardware version

---Get board inforamtion.
---@param type BoardInfoType
---@return string
function board.getInfo(type) end

return board
