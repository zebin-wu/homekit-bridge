---@class cipherlib
local cipher = {}

---@alias CipherType
---| '"None"'
---| '"AES-128-CBC"'

---@alias CipherPadding
---|>'"NONE"'           # Never pad (full blocks only).
---| '"PKCS7"'          # PKCS7 padding (default).
---| '"ISO7816_4"'      # ISO/IEC 7816-4 padding.
---| '"ANSI923"'        # ANSI X.923 padding.
---| '"ZERO"'           # Zero padding (not reversible).

---@class _cipher:userdata Cipher context.
local _cipher = {}

---Create a cipher context.
---@param type CipherType Cipher type.
---@return _cipher context Cipher context.
function cipher.create(type) end

---Return the length of the key in bytes.
---@return integer bytes The key length.
function _cipher:getKeyLen() end

---Return the length of the initialization vector (IV) in bytes.
---@return integer bytes The IV length.
function _cipher:getIVLen() end

---Set the padding mode, for cipher modes that use padding.
---@param padding CipherPadding The padding mode.
---@return boolean status true on success, false on failure.
function _cipher:setPadding(padding) end

---Begin a encryption/decryption process.
---@param op '"encrypt"'|'"decrypt"' Operation.
---@param key string The key to use.
---@param iv? string The initialization vector (IV).
---@return boolean status true on success, false on failure.
function _cipher:begin(op, key, iv) end

---The generic cipher update function.
---@param input string Input binary data.
---@return string|nil output Output binary data.
function _cipher:update(input) end

---The generic cipher finalization function.
---@return string|nil output Output binary data.
function _cipher:finsh() end

return cipher
