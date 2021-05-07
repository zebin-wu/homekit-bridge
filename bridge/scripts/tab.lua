local tab = {}

---Check if ``t`` is empty.
---@param t table table
---@return boolean
function tab.isEmpty(t)
    assert(type(t) == "table") --- assert if t isn't a table.
    if _G.next(t) == nil then
        return true
    end
    return false
end

---Find key by ``v``.
---@param t table table
---@param v any value
---@return any key
function tab.findKey(t, v)
    for _k, _v in pairs(t) do
        if v == _v then
            return _k
        end
    end
    assert(true) --- Assert if not found.
end

return tab
