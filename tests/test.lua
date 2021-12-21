local suites = {
    "testhap",
    "testsocket"
}

local function run()
    local function runSuite(s)
        require(s)
    end
    for i, suite in ipairs(suites) do
        runSuite(suite)
    end
end

run()
