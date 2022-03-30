---@meta

---@class hashlib
local M = {}

---@class HashContext:userdata Hash context.
local hash = {}

---@alias HashType
---|'"MD4"'
---|'"MD5"'
---|'"SHA1"'
---|'"SHA224"'
---|'"SHA256"'
---|'"SHA384"'
---|'"SHA512"'
---|'"RIPEMD160"'

---Update hash context with binary data.
---@param data string
---@return HashContext ctx
function hash:update(data) end

---Return the digest of the data passed to the update() method so far.
---@return string digest
---@nodiscard
function hash:digest() end

---Like ``digest()`` except the digest is returned as a string object of
---double length, containing only hexadecimal digits.
---@return string hexdigest
---@nodiscard
function hash:hexdigest() end

---Create a hash context.
---@param type HashType
---@return HashContext ctx
---@nodiscard
function M.create(type) end

return M
