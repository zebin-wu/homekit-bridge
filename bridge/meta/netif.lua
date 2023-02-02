---@meta

---@class netiflib Network interface library.
local M = {}

---@alias NetIfEvent
---|'"Added"'
---|'"Removed"'
---|'"Up"'
---|'"Down"'
---|'"Ipv4Changed"'
---|'"Ipv6Changed"'

---@class netif Network interface.

---Get current network interfaces.
---@return netif[]
function M.getInterfaces() end

---Find a interface by name.
---@param name string
---@return netif
function M.find(name) end

---Whether the interface is up.
---@param netif netif
---@return boolean
function M.isUp(netif) end

---Got IPv4 address.
---@param netif netif
---@return string
function M.getIpv4Addr(netif) end

---Got IPv6 addresses.
---@param netif netif
---@return string[]
function M.getIpv6Addrs(netif) end

---Wait for event to arrive.
---@param events NetIfEvent[]
---@param netif? netif
---@return netif
---@return NetIfEvent
function M.wait(events, netif) end

return M
