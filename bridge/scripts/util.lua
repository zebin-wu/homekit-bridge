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

---Serialize a Lua value.
---@param v any Lua value
---@return string
local function serialize(v)
    local t = type(v)
    if t == "string" then
        return "\"" .. v .. "\""
    elseif t == "table" then
        local s = "{"
        for _k, _v in pairs(v) do
            if type(_k) == "string" then
                s = s .. _k .. ": "
            end
            s = s .. ("%s, "):format(serialize(_v))
        end
        s = s .. "}"
        return s
    else
        return tostring(v)
    end
end

util.serialize = serialize

return util
