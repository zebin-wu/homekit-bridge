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

---Split a string.
---@param s string
---@param c string
---@return table results
function util.split(s, c)
    local rl = {}
    local tinsert = table.insert
    for w in s:gmatch("[^" .. c .. "]+") do
        tinsert(rl, w)
    end
    return rl
end

---Hex to binary.
---@param s string Hex string.
---@return string # Binary string.
function util.hex2bin(s)
    return s:gsub("..", function (cc)
        return string.char(tonumber(cc, 16))
    end)
end

---Binary to hex.
---@param s string Binary string.
---@return string # Hex string.
function util.bin2hex(s)
    return s:gsub(".", function (c)
        return ("%02X"):format(string.byte(c))
    end)
end

return util
