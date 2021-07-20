---@class hashlib
local hash = {}

---@class _hash:userdata Hash object.
local _hash = {}

---Update hash object with binary string.
---@param data string Binary string.
function _hash:update(data) end

---Return the digest of the data passed to the update() method so far.
---@return string digest
function _hash:digest() end

---New a MD5 (128-bits) hash object.
---@return _hash _hash
function hash.md5() end

return hash
