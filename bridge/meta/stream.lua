---@meta

---@class streamlib Stream library.
local M = {}

---@class StreamClient:userdata Stream client.
local client = {}

---Set the timeout.
---@param ms integer Maximum time blocked in milliseconds.
function client:settimeout(ms) end

---Write data.
---@param data string The data to be write.
function client:write(data) end

---Read data.
---@param maxlen integer The max length of the data.
---@param all? boolean Read all data.
---@return string data The read data.
---@nodiscard
function client:read(maxlen, all) end

---Read all data.
---The function will block until timeout or ``EOF`` is received.
---Timeout does not raise error.
---@return string data The read data.
function client:readall() end

---Read a line.
---@param sep? string Separator, default is "\n".
---@param skip? boolean Whether to skip the separator, default is false.
---@return string line
function client:readline(sep, skip) end

---Close the connection.
function client:close() end

---Create a stream client and connect to the host.
---@param type '"TCP"'|'"TLS"'|'"DTLS"'
---@param host string Server host name or IP address.
---@param port integer Remote port number, in host order.
---@param timeout integer Timeout period (in milliseconds).
---@return StreamClient
---@nodiscard
function M.client(type, host, port, timeout) end

return M
