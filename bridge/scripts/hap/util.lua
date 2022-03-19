local hap = require "hap"

local M = {}

---Get bridged accessory Instance ID.
---@param handle NVSHandle
---@return integer
function M.getBridgedAccessoryIID(handle)
    local aid = handle:get("aid")
    if aid == nil then
        aid = hap.getNewInstanceID(true)
        handle:set("aid", aid)
        handle:commit()
    end
    return aid
end

---Get Instance IDs for services or characteristics, excluding bridged accessories.
---@param handle NVSHandle
---@return table<string, integer>
function M.getInstanceIDs(handle)
    local iids = handle:get("iids")
    if iids == nil then
        iids = {}
    end
    local mt = {}
    function mt:__index(k)
        local v = iids[k]
        if v == nil then
            v = hap.getNewInstanceID()
            iids[k] = v
            handle:set("iids", iids)
            handle:commit()
        end
        return v
    end
    function mt:__newindex(k, v)
        error("read only")
    end
    return setmetatable({}, mt)
end

return M
