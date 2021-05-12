local char = require "hap.char"

local logger = log.getLogger("testhap")

local function deepCopy(object)
    local lookup = {}
    local function _copy(o)
        if type(o) ~= "table" then
            return o
        elseif lookup[o] then
            return lookup[o]
        end

        local new = {}
        lookup[o] = new
        for k, v in pairs(o) do
            new[_copy(k)] = _copy(v)
        end
        return setmetatable(new, getmetatable(o))
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

---Test function ``f`` with each argument in ``args``.
---@param f function The function to test.
---@param args any[] Array of arguments.
local function testFn(f, args)
    for i, arg in ipairs(args) do
        f(arg)
        collectgarbage()
    end
end

---Test configure() with a accessory.
---@param expect boolean The expected value returned by configure().
---@param primary boolean Primary or bridged accessory.
---@param k string The key want to test.
---@param vals any[] Array of values.
---@param log? boolean
local function testAccessory(expect, primary, k, vals, log)
    assert(type(expect) == "boolean")
    assert(type(primary) == "boolean")
    assert(type(k) == "string")
    assert(type(vals) == "table")

    if type(log) ~= "boolean" then
        log = true
    end

    local t
    if primary then
        t = "primary"
    else
        t = "bridged"
    end

    local function _test(v)
        local accs = {
            primary = {
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
            },
            bridged = {
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
        }
        if k == "service" then
            table.insert(accs[t].services, v)
        else
            accs[t][k] = v
        end
        if log then
            logger:info(
                string.format("Testing configure() with %s accessory %s: %s = %s",
                    t, k, type(v), v)
            )
        end
        assert(hap.configure(accs.primary, { accs.bridged }, {}, false) == expect)
        hap.unconfigure()
    end
    testFn(_test, vals)
end

local function testService(expect, k, vals, log)
    if type(log) ~= "boolean" then
        log = true
    end
    local function _test(v)
        local service = {
            iid = hap.getNewInstanceID(),
            type = "LightBulb",
            props = {
                primaryService = true,
                hidden = false,
                ble = {
                    supportsConfiguration = false,
                }
            },
            chars = {
                char.newServiceSignatureCharacteristic(hap.getNewInstanceID()),
                char.newServiceSignatureCharacteristic(hap.getNewInstanceID())
            }
        }
        service[k] = v
        if log then
            logger:info(
                string.format("Testing configure() with service %s: %s = %s",
                    k, type(v), v)
            )
        end
        testAccessory(expect, false, "service", { service }, false)
    end
    testFn(_test, vals)
end

---Configure with valid accessory IID.
---Primary accessory must have aid 1.
---Bridged accessory must have aid other than 1.
testAccessory(true, true, "aid", { 1 })
testAccessory(true, false, "aid", { 2 })

---Configure with invalid accessory IID.
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

---Configure with valid accessory cbs.
testAccessory(true, true, "cbs", { {}, { identify = function () end}})

---Configure with invalid accessory cbs.
testAccessory(false, true, "cbs", { nil })

---Configure with invalid service IID.
testService(false, "iid", { -1 })
