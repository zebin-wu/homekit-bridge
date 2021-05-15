local util = {}

---Check if table ``t`` is empty.
---@param t table table
---@return boolean
function util.isEmptyTable(t)
    return next(t) == nil
end

---Search for the first key in table `t` by the value ``v``.
---@param t table table
---@param v any value
---@return any key
function util.searchKey(t, v)
    for _k, _v in pairs(t) do
        if v == _v then
            return _k
        end
    end
    return nil
end

return util
