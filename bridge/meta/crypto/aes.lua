---@class aeslib
local aes = {}

---@class _aes:userdata AES context.
local _aes = {}

---Create an AES context for CBC mode.
---@param key string Binary AES key.
---@param iv string Binary initialization vector.
---@return _aes context AES context.
function aes.create(key, iv) end

---Encrypt data with AES.
---@param input string Binary input data.
---@return string output Binary output data.
function _aes:encrypt(input) end

---Decrypt data with AES.
---@param input string Binary input data.
---@return string output Binary output data.
function _aes:decrypt(input) end

return aes
