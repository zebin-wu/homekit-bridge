---@class hashlib
local hash = {}

---@class _hash:userdata Hash object.
local _hash = {}

---@alias HashType
---|'"MD4"'
---|'"MD5"'
---|'"SHA1"'
---|'"SHA224"'
---|'"SHA256"'
---|'"SHA384"'
---|'"SHA512"'
---|'"RIPEMD160"'

---Update hash object with binary data.
---@param data string Binary data.
function _hash:update(data) end

---Return the digest of the data passed to the update() method so far.
---@return string digest
---@nodiscard
function _hash:digest() end

---New a hash object.
---@param type HashType
---@return _hash _hash
---@nodiscard
function hash.new(type) end

return hash
