---@class aeslib
local aes = {}

---@class _cbc:userdata AES CBC context.
local _cbc = {}

---Create an AES context for CBC mode.
---@param opt '"encrypt"' | '"decrypt"' AES operation.
---@param key string Binary AES key.
---@param iv string Binary initialization vector.
---@return _cbc context AES CBC context.
function aes.cbc(opt, key, iv) end

---Get initialization vector.
---@return string iv Binary initialization vector.
function _cbc:getIV() end

---Set initialization vector.
---@param iv string Binary initialization vector.
function _cbc:setIV(iv) end

---Encrypt data with AES.
---@param input string Binary input data.
---@return string output Binary output data.
function _cbc:encrypt(input) end

---Decrypt data with AES.
---@param input string Binary input data.
---@return string output Binary output data.
function _cbc:decrypt(input) end

return aes
