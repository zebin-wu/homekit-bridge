---@meta

---@class hashlib
local hash = {}

---@class HashContext:userdata Hash context.
local ctx = {}

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
---@param data string Binary data.
function ctx:update(data) end

---Return the digest of the data passed to the update() method so far.
---@return string digest
---@nodiscard
function ctx:digest() end

---Create a hash context.
---@param type HashType
---@return HashContext ctx
---@nodiscard
function hash.create(type) end

return hash
