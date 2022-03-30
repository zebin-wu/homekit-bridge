---@meta

---@class arc4lib ARC4 stream library.
local M = {}

---@class ARC4Context:userdata ARC4 context.
local ctx = {}

---Encrypt/Decrypt data.
---@param input string
---@return string output
function ctx:crypt(input) end

---Reset the context.
function ctx:reset() end

---Create a ARC4 context.
---@param key string Secret key, the key length is in range ``(5, 255)``.
---@param ndrop? integer Number bytes to drop.
---@return ARC4Context context
function M.create(key, ndrop) end

return M
