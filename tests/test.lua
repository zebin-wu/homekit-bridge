local suites = {
    "testcore",
    "testsocket",
    "teststream",
    "testnvs",
    "testmiiotransport",
}

local function runSuite(s)
    require(s)
end
for i, suite in ipairs(suites) do
    runSuite(suite)
end
