local nvs = require "nvs"

-- Tests nvs.open() with valid parameters.
for _, mode in ipairs({"r", "rw"}) do
    local handle <close> = nvs.open("test", mode)
end
do
    local handle <close> = nvs.open("test")
end

-- Tests nvs.open() with invalid namespace.
do
    local success = pcall(nvs.open, nil)
    assert(success == false)
end

-- Tests nvs.open() twice with read only mode.
do
    local handle1 <close> = nvs.open("test", "r")
    local handle2 <close> = nvs.open("test", "r")
end

-- Tests nvs.get() with valid parameters.
do
    local handle <close> = nvs.open("test")
    handle:get("test")
end

-- Tests nvs.get() with invalid parameters.
do
    local handle <close> = nvs.open("test")
    local success = pcall(handle.get, handle, nil)
    assert(success == false)
end

-- Tests nvs.set() with valid parameters.
do
    local handle <close> = nvs.open("test")
    for _, value in ipairs({true, 1, 1.1, "hello world", nil}) do
        handle:set("test", value)
        assert(handle:get("test") == value)
    end

    local arr = {1, 2, 3, 4}
    handle:set("test", arr)
    for i, v in ipairs(handle:get("test")) do
        assert(arr[i] == v)
    end

    local arr = {a = 1, b = true, c = "hello world"}
    handle:set("test", arr)
    for k, v in pairs(handle:get("test")) do
        assert(arr[k] == v)
    end
end

-- Tests nvs.set() with invalid parameters.
do
    local handle <close> = nvs.open("test")
    local success = pcall(handle.set, handle, nil)
    assert(success == false)

    for _, value in ipairs({function () end}) do
        local success,result = pcall(handle.set, handle, "test", value)
        assert(success == false)
    end
end

-- Tests nvs.erase() after setting key.
do
    local handle <close> = nvs.open("test")
    handle:set("test", 1)
    handle:erase()
    assert(handle:get("test") == nil)
end

-- Tests nvs.commit() without changing anything.
do
    local handle <close> = nvs.open("test")
    handle:commit()
end

-- Tests nvs.commit() after setting key.
do
    local handle <close> = nvs.open("test")
    handle:set("test", 1)
    handle:commit()
end

-- Tests nvs.close() with a <close> handle.
do
    local handle <close> = nvs.open("test")
    handle:close()
end

-- Tests if the fetched value is equal to the value set before reopening.
do
    local values = {1, true, "hello world"}
    for i, v in ipairs(values) do
        do
            local handle <close> = nvs.open("test")
            handle:set("test" .. i, v)
        end
    end
    for i, v in ipairs(values) do
        do
            local handle <close> = nvs.open("test")
            assert(handle:get("test" .. i) == v)
        end
    end
end

do
    local handle <close> = nvs.open("test")
    handle:erase()
end
