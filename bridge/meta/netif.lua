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

---Wait for event to arrive.
---@param event NetIfEvent
---@param netif? netif
---@return netif
function M.wait(event, netif) end

---Whether the interface is up.
---@return boolean
function M.isUp(netif) end

---Got IPv4 address.
---@return string
function M.getIpv4Addr(netif) end

---Got IPv6 addresses.
---@return string[]
function M.getIpv6Addrs(netif) end

return M
