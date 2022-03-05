local nvs = require "nvs"

-- Tests nvs.open() with valid parameters.
for _, name in ipairs({"test", "123456789012345"}) do
    local handle <close> = nvs.open(name)
end

-- Tests nvs.open() with invalid namespace.
do
    local success = pcall(nvs.open, nil)
    assert(success == false)
end
for _, name in ipairs({"", "1234567890123456"}) do
    local success = pcall(nvs.open, name)
    assert(success == false)
end

-- Tests nvs.open() twice.
do
    local handle1 <close> = nvs.open("test")
    local handle2 <close> = nvs.open("test")
end

-- Tests nvs.get() with valid parameters.
do
    local handle <close> = nvs.open("test")
    for _, key in ipairs({"test", "123456789012345"}) do
        handle:get(key)
    end
end

-- Tests nvs.get() with invalid parameters.
do
    local handle <close> = nvs.open("test")
    local success = pcall(handle.get, handle, nil)
    assert(success == false)
    for _, key in ipairs({"", "1234567890123456"}) do
        success = pcall(handle.get, handle, key)
        assert(success == false)
    end
end

-- Tests nvs.set() with valid parameters.
do
    local handle <close> = nvs.open("test")
    for _, key in ipairs({"test", "123456789012345"}) do
        handle:set(key, nil)
    end
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
    for _, key in ipairs({"", "1234567890123456"}) do
        success = pcall(handle.set, handle, key, nil)
        assert(success == false)
    end
    for _, value in ipairs({function () end}) do
        success = pcall(handle.set, handle, "test", value)
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
