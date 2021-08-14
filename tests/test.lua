local suites = {
    "testhap"
}

local function run()
    local function runSuite(s)
        require(s)
    end
    for i, suite in ipairs(suites) do
        runSuite(suite)
    end
    return true
end

return run()
