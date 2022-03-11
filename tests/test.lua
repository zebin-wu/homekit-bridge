local core = require "core"

local suites = {
    "testhap",
    "testsocket",
    "testnvs"
}

local function runSuite(s)
    require(s)
end
for i, suite in ipairs(suites) do
    runSuite(suite)
end

core.exit()
