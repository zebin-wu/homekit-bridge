---@meta

local M = {}

---@class SSLContext:userdata SSL context.
local ctx = {}

---Whether the handshake is finshed.
---@return boolean finshed
---@nodiscard
function ctx:finshed() end

---Perform the SSL handshake.
---@param input? string
---@return string output
---@nodiscard
function ctx:handshake(input) end

---Encrypt data to be output.
---@param input string
---@return string output
---@nodiscard
function ctx:encrypt(input) end

---Decrypt input data.
---@param input string
---@return string output
---@nodiscard
function ctx:decrypt(input) end

---Create a SSL context.
---@param type '"TLS"'|'"DTLS"' SSL type.
---@param endpoint '"client"'|'"server"' SSL endpoint.
---@param hostname? string host name, only valid when the SSL endpoint is "client".
---@return SSLContext context
---@nodiscard
function M.create(type, endpoint, hostname) end

return M
