---@meta

local ssl = {}

---@class SSLCtx:userdata SSL context.
local ctx = {}

---Create a SSL context.
---@param endpoint '"client"'|'"server"' SSL endpoint.
---@param hostname? string host name, only valid when the SSL endpoint is "client".
---@return SSLCtx context
function ssl.create(endpoint, hostname) end

---Whether the handshake is finshed.
---@return boolean finshed
function ctx:finshed() end

---Perform the SSL handshake.
---@param input? string
---@return string output
function ctx:handshake(input) end

---Encrypt data to be output.
---@param input string
---@return string output
function ctx:encrypt(input) end

---Decrypt input data.
---@param input string
---@return string output
function ctx:decrypt(input) end

return ssl
