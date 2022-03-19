---@meta

---@class dnslib
local M = {}

---Resolve host name.
---@param hostname string Host name.
---@param timeout integer Timeout period (in milliseconds).
---@param family? '"IPV4"'|'"IPV6"' Address family.
---@return string addr The resolved address.
---@return '"IPV4"'|'"IPV6"' family Address family.
function M.resolve(hostname, timeout, family) end

return M
