local nvs = require "nvs"

local M = {}

---Get value.
---@param key string
---@return any
function M.get(key)
    local handle <close> = nvs.open("bridge::conf")
    local value = handle:get(key)

    if type(value) == "table" then
        return value[1]
    end
    return value
end

---Get all values.
function M.getall(key)
    local handle <close> = nvs.open("bridge::conf")
    local value = handle:get(key)
    local t = type(value)

    if type(value) == "string" then
        return { value }
    end
    return value
end

---Set a value.
---@param key string
---@param value any
function M.set(key, value)
    local handle <close> = nvs.open("bridge::conf")
    handle:set(key, value)
    handle:commit()
end

---Add a value.
---@param key string
---@param value string
function M.add(key, value)
    local values = M.getall(key)
    table.insert(values, value)
    M.set(key, values)
end

---Unset a item.
---@param key any
function M.unset(key)
    M.set(key, nil)
end

---Reset all items.
function M.reset()
    local handle <close> = nvs.open("bridge::conf")
    handle:erase()
end

local function help()
    print("usage: conf [option] [key [value]]")
end

local function checkop(op)
    if op then
        error("already has operation: " .. op)
    end
end

function M.main(...)
    local args = {...}
    if #args == 0 then
        help()
        return
    end
    local option = true
    local operate
    local key
    local value
    for _, arg in ipairs(args) do
        if option then
            if arg:sub(1, 2) == "--" then
                if arg == "--unset" then
                    checkop(operate)
                    operate = M.unset
                elseif arg == "--add" then
                    checkop(operate)
                    operate = M.add
                elseif arg == "--get" then
                    checkop(operate)
                    operate = M.get
                elseif arg == "--get-all" then
                    checkop(operate)
                    operate = M.getall
                else
                    error("unkown option: " .. arg)
                end
            else
                option = false
            end
        end
        if not option then
            if key == nil then
                key = arg
            elseif value == nil then
                value = arg
            else
                error("invalid arguments")
            end
        end
    end

    if not key then
        error("missing key")
    end

    if operate == M.getall then
        for _, v in ipairs(M.getall(key)) do
            print(v)
        end
    elseif operate == nil then
        if value ~= nil then
            M.set(key, value)
        else
            local value = M.get(key)
            if value then
                print(value)
            end
        end
    else
        operate(key, value)
    end
end

return M
