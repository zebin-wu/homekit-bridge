---@meta

---@class jsonlib
---
---@field null lightuserdata A Lua lightuserdata ``NULL`` pointer.
local M = {}

---Deserialise any UTF-8 JSON string into a Lua value or table.
---UTF-16 and UTF-32 JSON strings are not supported.
---@param s string JSON format string.
---@return any v Lua value or table.
---@nodiscard
function M.decode(s) end

return M
