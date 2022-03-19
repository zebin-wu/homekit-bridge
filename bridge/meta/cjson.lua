---@meta

---@class cjsonlib
---
---@field null lightuserdata A Lua lightuserdata ``NULL`` pointer.
local M = {}

---Deserialise any UTF-8 JSON string into a Lua value or table.
---UTF-16 and UTF-32 JSON strings are not supported.
---@param s string JSON format string.
---@return any v Lua value or table.
---@nodiscard
function M.decode(s) end

---Serialise a Lua value into a string containing the JSON representation.
---@param v any Lua value or table.
---@return string s JSON format string.
---@nodiscard
function M.encode(v) end

---Instantiate an independent copy of the Lua CJSON module.
---The new module has a separate persistent encoding buffer, and default settings.
---@return cjsonlib cjson cjson module.
---@nodiscard
function M.new() end

return M
