local logger = log.getLogger("testhap")

logger:info("Testing ...")

local defPrimaryAcc = {
    aid = 1, -- Primary accessory must have aid 1.
    category = "Bridges",
    name = "test",
    mfg = "mfg1",
    model = "model1",
    sn = "1234567890",
    fwVer = "1",
    services = {
        hap.AccessoryInformationService,
        hap.HapProtocolInformationService,
        hap.PairingService,
    }
}

local defBridgedAcc = {
    aid = 2,
    category = "BridgedAccessory",
    name = "test",
    mfg = "mfg1",
    model = "model1",
    sn = "1234567890",
    fwVer = "1",
    services = {
        hap.AccessoryInformationService,
        hap.HapProtocolInformationService,
        hap.PairingService,
    }
}

local function deepCopy(object)
    local lookup_table = {}
    local function _copy(object)
        if type(object) ~= "table" then
            return object
        elseif lookup_table[object] then
            return lookup_table[object]
        end

        local new_table = {}
        lookup_table[object] = new_table
        for key, value in pairs(object) do
            new_table[_copy(key)] = _copy(value)
        end
        return setmetatable(new_table, getmetatable(object))
    end

    return _copy(object)
end

local function fillStr(n, fill)
    fill = fill or "0123456789"
    local s = ""
    while #s < n - #fill do
        s = s .. fill
    end
    return s .. string.sub(fill, 0, n - #s)
end

---Test configure() with a accessory.
---@param expect boolean The expected value returned by configure().
---@param primary boolean Primary or bridged accessory.
---@param k string The key want to test.
---@param vals any[] Array of values.
local function testAccessory(expect, primary, k, vals)
    assert(type(expect) == "boolean")
    assert(type(primary) == "boolean")
    assert(type(k) == "string")
    assert(type(vals) == "table")

    local function _test(v)
        local primaryAcc = defPrimaryAcc
        local bridgedAcc = defBridgedAcc
        local accType
        if primary then
            primaryAcc = deepCopy(defPrimaryAcc)
            primaryAcc[k] = v
            accType = "primary"
        else
            bridgedAcc = deepCopy(defBridgedAcc)
            bridgedAcc[k] = v
            accType = "bridged"
        end
        logger:info(string.format("Testing configure() with %s accessory %s: %s = %s", accType, k, type(v), v))
        assert(hap.configure(primaryAcc, { bridgedAcc }, {}, false) == expect)
        hap.unconfigure()
    end
    for i, v in ipairs(vals) do
        _test(v)
        collectgarbage()
    end
end

---Configure with default accessory.
assert(hap.configure(defPrimaryAcc, { defBridgedAcc }, {}, false) == true)
hap.unconfigure()

---Configure with invalid accessory IID.
---Primary accessory must have aid 1.
---Bridged accessory must have aid other than 1.
testAccessory(false, true, "aid", { -1, 0, 2, "1", {} })
testAccessory(false, false, "aid", { 1 })

---Configure with valid accessory category.
testAccessory(true, true, "category", {
    "BridgedAccessory",
    "Other",
    "Bridges",
    "Fans",
    "GarageDoorOpeners",
    "Lighting",
    "Locks",
    "Outlets",
    "Switches",
    "Thermostats",
    "Sensors",
    "SecuritySystems",
    "Doors",
    "Windows",
    "WindowCoverings",
    "ProgrammableSwitches",
    "RangeExtenders",
    "IPCameras",
    "AirPurifiers",
    "Heaters",
    "AirConditioners",
    "Humidifiers",
    "Dehumidifiers",
    "Sprinklers",
    "Faucets",
    "ShowerSystems"
})

---Configure with invalid accessory category.
testAccessory(false, true, "category",  { nil, "", "category1", {}, true, 1 })
testAccessory(false, false, "category", {
    "Other",
    "Bridges",
    "Fans",
    "GarageDoorOpeners",
    "Lighting",
    "Locks",
    "Outlets",
    "Switches",
    "Thermostats",
    "Sensors",
    "SecuritySystems",
    "Doors",
    "Windows",
    "WindowCoverings",
    "ProgrammableSwitches",
    "RangeExtenders",
    "IPCameras",
    "AirPurifiers",
    "Heaters",
    "AirConditioners",
    "Humidifiers",
    "Dehumidifiers",
    "Sprinklers",
    "Faucets",
    "ShowerSystems"
})

---Configure with valid accessory name
testAccessory(true, true, "name", { "", fillStr(64) })

---Configure with invalid accessory name.
testAccessory(false, true, "name", { nil, fillStr(64 + 1), {}, true, 1 })

---Configure with valid accessory manufacturer.
testAccessory(true, true, "mfg", { "", fillStr(64) })

---Configure with invalid accessory manufacturer.
testAccessory(false, true, "mfg", { nil, fillStr(64 + 1), {}, true, 1 })

---Configure with valid accessory model.
testAccessory(true, true, "model", { fillStr(1), fillStr(64) })

---Configure with invalid accessory model.
testAccessory(false, true, "model", { nil, "", fillStr(64 + 1), {}, true, 1 })

---Configure with valid accessory serial number.
testAccessory(true, true, "sn", { fillStr(2), fillStr(64) })

---Configure with invalid accessory serial number.
testAccessory(false, true, "sn", { nil, "", fillStr(1), fillStr(64 + 1), {}, true, 1 })

---Configure with invalid accessory firmware version.
testAccessory(false, true, "fwVer", { nil, {}, true, 1 })

---Configure with invalid accessory hardware version.
testAccessory(false, true, "hwVer", { nil, {}, true, 1 })
