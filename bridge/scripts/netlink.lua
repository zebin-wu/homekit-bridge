local netiflib = require "netif"

---@class netlinklib Network link library.
local M = {}

---Whether the netif is link up.
---@param netif netif
local function isLinkUp(netif)
    local addr = netiflib.getIpv4Addr(netif)
    return netiflib.isUp(netif) and addr ~= "0.0.0.0" and addr ~= "127.0.0.1"
end

---Whether the netlink is up.
---@return boolean
function M.isUp()
    local ifs = netiflib.getInterfaces()
    for _, netif in ipairs(ifs) do
        if isLinkUp(netif) then
            return true
        end
    end

    return false
end

---Wait for the netlink up.
function M.waitUp()
    while true do
        if isLinkUp(netiflib.wait("Ipv4Changed")) then
            return
        end
    end
end

return M
