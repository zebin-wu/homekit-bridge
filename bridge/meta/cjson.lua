---@class cjsonlib
---
---@field null lightuserdata A Lua lightuserdata ``NULL`` pointer.
---@field empty_array lightuserdata A lightuserdata, similar to ``cjson.null``, which will be encoded as an empty JSON Array by ``cjson.encode()``.
---@field array_mt lightuserdata When lua-cjson encodes a table with this metatable, it will systematically encode it as a JSON Array.
---@field empty_array_mt lightuserdata A metatable which can "tag" a table as a JSON Array in case it is empty.
local cjson = {}

---Deserialise any UTF-8 JSON string into a Lua value or table.
---UTF-16 and UTF-32 JSON strings are not supported.
---@param s string JSON format string.
---@return any v Lua value or table.
function cjson.decode(s) end

---Serialise a Lua value into a string containing the JSON representation.
---@param v any Lua value or table.
---@return string s JSON format string.
function cjson.encode(v) end

---Instantiate an independent copy of the Lua CJSON module.
---The new module has a separate persistent encoding buffer, and default settings.
---@return cjsonlib cjson cjson module.
function cjson.new() end

return cjson
