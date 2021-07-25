---@class cipherlib
local cipher = {}

---@alias CipherType
---| '"AES-128-CBC"'

---@alias PaddingMode
---|>'"NONE"'   # Never pad (full blocks only).
---| '"PKCS7"'  # PKCS#7 padding.

---@class _cipher:userdata Cipher context.
local _cipher = {}

---New a cipher context.
---@param type CipherType Cipher type.
function cipher.new(type) end

---Return the key length of the cipher.
---@return integer bits The length of the key.
function _cipher:getKeyBitLen() end

---Return the size of the IV or nonce of the cipher, in Bytes.
---@return integer bytes The size of the IV in bytes.
function _cipher:getIvSize() end

---Set the key to use.
---@param opt '"encrypt"'|'"decrypt"' Operation.
---@param key string The key to use.
function _cipher:setKey(opt, key) end

---Set the padding mode, for cipher modes that use padding.
---@param mode PaddingMode The padding mode.
function _cipher:setPaddingMode(mode) end

---Set the initialization vector (IV) or nonce.
---@param iv string The initialization vector (IV).
function _cipher:setIv(iv) end

---The generic cipher update function.
---@param input string Input binary data.
---@return string output Output binary data.
function _cipher:update(input) end

---The generic cipher finalization function.
---@return string output Output binary data.
function _cipher:finsh() end

return cipher
