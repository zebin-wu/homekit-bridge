---@meta

---@class dnslib
local dns = {}

---Resolve host name.
---@param hostname string Host name.
---@param timeout integer Timeout period (in milliseconds).
---@param famliy? '"IPV4"'|'"IPV6"' Address famliy.
---@return string addr The resolved address.
---@return '"IPV4"'|'"IPV6"' family Address famliy.
function dns.resolve(hostname, timeout, famliy) end

return dns
