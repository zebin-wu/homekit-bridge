local hap = require "hap"
local util = require "util"

local logger = log.getLogger("testhap")

local function fillStr(n, fill)
    fill = fill or "0123456789"
    local s = ""
    while #s < n - #fill do
        s = s .. fill
    end
    return s .. fill:sub(0, n - #s)
end

---Test function ``f`` with each argument in ``args``.
---@param f function The function to test.
---@param args any[] Array of arguments.
local function testFn(f, args)
    for i, arg in ipairs(args) do
        f(arg)
    end
end

---Log test information.
---@param fn string Function name.
---@param t string The table to test.
---@param k string Key.
---@param v any Value.
---@param e any The expected value returned by ``fn``.
local function logTestInfo(fn, t, k, v, e)
    logger:info(("Testing %s() with %s.%s: %s = %s, expected: %s"):format(fn, t, k, type(v), util.serialize(v), e))
end

local function setField(t, k, v)
    local rl = util.split(k, ".")
    for i = 1, #rl - 1, 1 do
        t = t[rl[i]]
    end
    t[rl[#rl]] = v
end

---Test init() with a accessory.
---@param expect boolean The expected value returned by init().
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
                },
                cbs = {}
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
                },
                cbs = {}
            }
        }
        if log then
            logTestInfo("init", t .. "Accessory", k, v, expect)
        end
        if k == "service" then
            table.insert(accs[t].services, v)
        else
            setField(accs[t], k, v)
        end
        local status = hap.init(accs.primary, { accs.bridged }, {
            updatedState = function (state) end
        })
        assert(status == expect, ("assertion failed: init() return %s, except %s"):format(status, expect))
        if status then hap.deinit() end
    end
    testFn(_test, vals)
end

---Test init() with a service.
---@param expect boolean The expected value returned by init().
---@param k string The key want to test.
---@param vals any[] Array of values.
---@param log? boolean
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
                require("hap.char.ServiceSignature").new(hap.getNewInstanceID()),
                require("hap.char.Name").new(hap.getNewInstanceID())
            }
        }
        if log then
            logTestInfo("init", "service", k, v, expect)
        end
        if k == "char" then
            table.insert(service.chars, v)
        else
            setField(service, k, v)
        end
        testAccessory(expect, false, "service", { service }, false)
    end
    testFn(_test, vals)
end

---Test init() with a characteristic.
---@param expect boolean The expected value returned by init().
---@param k string The key want to test.
---@param vals any[] Array of values.
---@param format? HapCharacteristicFormat Characteristic format.
---@param constraints? HapStringCharacteristiConstraints|HapNumberCharacteristiConstraints|HapUInt8CharacteristiConstraints Value constraints.
local function testCharacteristic(expect, k, vals, format, constraints)
    format = format or "Bool"
    local tab = "char"
    if format then
        tab = ("char[%s]"):format(format)
    end
    local function _test(v)
        local c = {
            format = format,
            iid = hap.getNewInstanceID(),
            type = "On",
            props = {
                readable = true,
                writable = true,
                supportsEventNotification = true,
                hidden = false,
                requiresTimedWrite = false,
                supportsAuthorizationData = false,
                ip = { controlPoint = false, supportsWriteResponse = false },
                ble = {
                    supportsBroadcastNotification = true,
                    supportsDisconnectedNotification = true,
                    readableWithoutSecurity = false,
                    writableWithoutSecurity = false
                }
            },
            constraints = constraints or {},
            cbs = {
                read = function (request, context) end,
                write = function (request, value, context) end
            }
        }
        logTestInfo("init", tab, k, v, expect)
        setField(c, k, v)
        testService(expect, "char", { c }, false)
    end
    testFn(_test, vals)
end

---Initialize with valid accessory IID.
---Primary accessory must have aid 1.
---Bridged accessory must have aid other than 1.
testAccessory(true, true, "aid", { 1 })
testAccessory(true, false, "aid", { 2 })

---Initialize with invalid accessory IID.
testAccessory(false, true, "aid", { -1, 0, 1.1, 2, "1", {} })
testAccessory(false, false, "aid", { 1 })

---Initialize with valid accessory category.
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

---Initialize with invalid accessory category.
testAccessory(false, true, "category",  { "", "category1", {}, true, 1 })
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

---Initialize with valid accessory name
testAccessory(true, false, "name", { "", "附件名称", fillStr(64) })

---Initialize with invalid accessory name.
testAccessory(false, false, "name", { fillStr(64 + 1), {}, true, 1 })

---Initialize with valid accessory manufacturer.
testAccessory(true, false, "mfg", { "", fillStr(64) })

---Initialize with invalid accessory manufacturer.
testAccessory(false, false, "mfg", { fillStr(64 + 1), {}, true, 1 })

---Initialize with valid accessory model.
testAccessory(true, false, "model", { fillStr(1), fillStr(64) })

---Initialize with invalid accessory model.
testAccessory(false, false, "model", { "", fillStr(64 + 1), {}, true, 1 })

---Initialize with valid accessory serial number.
testAccessory(true, false, "sn", { fillStr(2), fillStr(64) })

---Initialize with invalid accessory serial number.
testAccessory(false, false, "sn", { "", fillStr(1), fillStr(64 + 1), {}, true, 1 })

---Initialize with invalid accessory firmware version.
testAccessory(false, false, "fwVer", { {}, true, 1 })

---Initialize with invalid accessory hardware version.
testAccessory(false, false, "hwVer", { {}, true, 1 })

---Initialize with valid accessory cbs.
testAccessory(true, false, "cbs", { {} })

---Initialize with invalid accessory cbs.
testAccessory(false, false, "cbs", { true, 1, "test" })

---Initialize with valid accessory identify callback.
testAccessory(true, false, "cbs.identify", { function () end })

---Initialize with invalid accessory identify calback.
testAccessory(false, false, "cbs.identify", { true, 1, "test", {} })

---Initialize with invalid service IID.
testService(false, "iid", { -1, 1.1, {}, true })

---Initialize with valid service type.
testService(true, "type", {
    "AccessoryInformation",
    "GarageDoorOpener",
    "LightBulb",
    "LockManagement",
    "LockMechanism",
    "Outlet",
    "Switch",
    "Thermostat",
    "Pairing",
    "SecuritySystem",
    "CarbonMonoxideSensor",
    "ContactSensor",
    "Door",
    "HumiditySensor",
    "LeakSensor",
    "LightSensor",
    "MotionSensor",
    "OccupancySensor",
    "SmokeSensor",
    "StatelessProgrammableSwitch",
    "TemperatureSensor",
    "Window",
    "WindowCovering",
    "AirQualitySensor",
    "BatteryService",
    "CarbonDioxideSensor",
    "HAPProtocolInformation",
    "Fan",
    "Slat",
    "FilterMaintenance",
    "AirPurifier",
    "HeaterCooler",
    "HumidifierDehumidifier",
    "ServiceLabel",
    "IrrigationSystem",
    "Valve",
    "Faucet",
    "CameraRTPStreamManagement",
    "Microphone",
    "Speaker",
})

---Initialize with invalid service type.
testService(false, "type", { "type1", "", {}, true, 1 })

---Initialize with valid service props.
testService(true, "props", { {} })

---Initialize with invalid service props.
testService(false, "props", { "test", true, 1 })

---Initialize with valid service proeprty ble.
testService(true, "props.ble", { {} })

---Initialize with invalid service property ble.
testService(false, "props.ble", { "test", true, 1 })


for i, k in ipairs({ "primaryService", "hidden", "ble.supportsConfiguration" }) do
    ---Initialize with invalid service property ``k``.
    testService(false, "props." .. k, { {}, 1, "test" })
end

---Initialize with invalid characteristic iid.
testCharacteristic(false, "iid", { -1, 1.1, true, {} })

---Configrue with valid characteristic format.
testCharacteristic(true, "format", {
    "Data",
    "Bool",
    "UInt8",
    "UInt16",
    "UInt32",
    "UInt64",
    "Int",
    "Float",
    "String",
    "TLV8",
})

---Configrue with invalid characteristic format.
testCharacteristic(false, "format", { {}, 1, true, "test" })

---Initialize with valid characteristic type.
testCharacteristic(true, "type", {
    "AdministratorOnlyAccess",
    "AudioFeedback",
    "Brightness",
    "CoolingThresholdTemperature",
    "CurrentDoorState",
    "CurrentHeatingCoolingState",
    "CurrentRelativeHumidity",
    "CurrentTemperature",
    "HeatingThresholdTemperature",
    "Hue",
    "Identify",
    "LockControlPoint",
    "LockManagementAutoSecurityTimeout",
    "LockLastKnownAction",
    "LockCurrentState",
    "LockTargetState",
    "Logs",
    "Manufacturer",
    "Model",
    "MotionDetected",
    "Name",
    "ObstructionDetected",
    "On",
    "OutletInUse",
    "RotationDirection",
    "RotationSpeed",
    "Saturation",
    "SerialNumber",
    "TargetDoorState",
    "TargetHeatingCoolingState",
    "TargetRelativeHumidity",
    "TargetTemperature",
    "TemperatureDisplayUnits",
    "Version",
    "PairSetup",
    "PairVerify",
    "PairingFeatures",
    "PairingPairings",
    "FirmwareRevision",
    "HardwareRevision",
    "AirParticulateDensity",
    "AirParticulateSize",
    "SecuritySystemCurrentState",
    "SecuritySystemTargetState",
    "BatteryLevel",
    "CarbonMonoxideDetected",
    "ContactSensorState",
    "CurrentAmbientLightLevel",
    "CurrentHorizontalTiltAngle",
    "CurrentPosition",
    "CurrentVerticalTiltAngle",
    "HoldPosition",
    "LeakDetected",
    "OccupancyDetected",
    "PositionState",
    "ProgrammableSwitchEvent",
    "StatusActive",
    "SmokeDetected",
    "StatusFault",
    "StatusJammed",
    "StatusLowBattery",
    "StatusTampered",
    "TargetHorizontalTiltAngle",
    "TargetPosition",
    "TargetVerticalTiltAngle",
    "SecuritySystemAlarmType",
    "ChargingState",
    "CarbonMonoxideLevel",
    "CarbonMonoxidePeakLevel",
    "CarbonDioxideDetected",
    "CarbonDioxideLevel",
    "CarbonDioxidePeakLevel",
    "AirQuality",
    "ServiceSignature",
    "AccessoryFlags",
    "LockPhysicalControls",
    "TargetAirPurifierState",
    "CurrentAirPurifierState",
    "CurrentSlatState",
    "FilterLifeLevel",
    "FilterChangeIndication",
    "ResetFilterIndication",
    "CurrentFanState",
    "Active",
    "CurrentHeaterCoolerState",
    "TargetHeaterCoolerState",
    "CurrentHumidifierDehumidifierState",
    "TargetHumidifierDehumidifierState",
    "WaterLevel",
    "SwingMode",
    "TargetFanState",
    "SlatType",
    "CurrentTiltAngle",
    "TargetTiltAngle",
    "OzoneDensity",
    "NitrogenDioxideDensity",
    "SulphurDioxideDensity",
    "PM2_5Density",
    "PM10Density",
    "VOCDensity",
    "RelativeHumidityDehumidifierThreshold",
    "RelativeHumidityHumidifierThreshold",
    "ServiceLabelIndex",
    "ServiceLabelNamespace",
    "ColorTemperature",
    "ProgramMode",
    "InUse",
    "SetDuration",
    "RemainingDuration",
    "ValveType",
    "IsConfigured",
    "ActiveIdentifier",
    "ADKVersion",
})

---Initialize with invalid characteristic type.
testCharacteristic(false, "type", { {}, "test", true, 1 })

---Initialize with valid characteristic manufacturer description.
testCharacteristic(true, "mfgDesc", { "", "manufacturer description", "厂商描述" })

---Initialize with invalid characteristic manufacturer description.
testCharacteristic(false, "mfgDesc", { {}, 1, true })

---Initialize with valid characteristic props.
testCharacteristic(true, "props", { {} })

---Initialize with invalid characteristic props.
testCharacteristic(false, "props", { "test", true, 1 })

---Initialize with valid characteristic property ip.
testCharacteristic(true, "props.ip", { {} })

---Initialize with invalid characteristic property ip.
testCharacteristic(false, "props.ip", { "test", true, 1 })

---Initialize with valid characteristic property ble.
testCharacteristic(true, "props.ble", { {} })

---Initialize with invalid characteristic property ble.
testCharacteristic(false, "props.ble", { "test", true, 1 })

for i, k in ipairs({
    "readable",
    "writable",
    "supportsEventNotification",
    "hidden",
    "readRequiresAdminPermissions",
    "writeRequiresAdminPermissions",
    "requiresTimedWrite",
    "supportsAuthorizationData",
    "ip.controlPoint",
    "ip.supportsWriteResponse",
    "ble.supportsBroadcastNotification",
    "ble.supportsDisconnectedNotification",
    "ble.readableWithoutSecurity",
    "ble.writableWithoutSecurity"
}) do
    ---Initialize with invalid characteristic property ``k``.
    testCharacteristic(false, "props." .. k, { {}, 1, "test" })
end

local units = {
    "None",
    "Celsius",
    "ArcDegrees",
    "Percentage",
    "Lux",
    "Seconds"
}

---Initialize with valid units.
for i, fmt in ipairs({ "UInt8", "UInt16", "UInt32", "UInt64", "Int", "Float" }) do
    testCharacteristic(true, "units", units, fmt)
end

---Initialize with invalid units.
for i, fmt in ipairs({ "Data", "String", "TLV8", "Bool" }) do
    testCharacteristic(false, "units", { "None" }, fmt)
end
testCharacteristic(false, "units", { "test", {}, true, 1 }, "UInt8")

---Initialize with valid constraints.
testCharacteristic(true, "constraints", { {} })

---Initialize with invalid constraints.
testCharacteristic(false, "constraints", { true, "test", 1 })

local format = {
    UInt8 = {
        min = 0,
        max = 0xff
    },
    UInt16 = {
        min = 0,
        max = 0xffff
    },
    UInt32 = {
        min = 0,
        max = 0xffffffff
    },
    UInt64 = {
        min = 0,
        max = math.maxinteger
    },
    Int = {
        min = -2147483648,
        max = 2147483647
    }
}

---Initialize with valid constraints maxLen.
for i, fmt in ipairs({ "String", "Data" }) do
    testCharacteristic(true, "constraints.maxLen", { format.UInt32.min, (format.UInt32.min + format.UInt32.max) // 2, format.UInt32.max }, fmt)
end

---Initialize with invalid constraints maxLen.
testCharacteristic(false, "constraints.maxLen", { format.UInt32.max + 1, 1.1, format.UInt32.min - 1, "test", {}, false }, "String")
testCharacteristic(false, "constraints.maxLen", { 0 }, "Bool")

local function getDftNumberConstraints(fmt)
    return {
        minVal = format[fmt].min,
        maxVal = format[fmt].max,
        stepVal = 1
    }
end

---Initialize with valid/invalid constraints minVal, maxVal, stepVal.
for i, fmt in ipairs({ "UInt8", "UInt16", "UInt32", "UInt64", "Int" }) do
    for i, key in ipairs({ "minVal", "maxVal" }) do
        testCharacteristic(true, "constraints." .. key, { format[fmt].min, format[fmt].max }, fmt, getDftNumberConstraints(fmt))
        testCharacteristic(false, "constraints." .. key, { format[fmt].min - 1, format[fmt].max + 1, "test", {}, false }, fmt, getDftNumberConstraints(fmt))
    end
    testCharacteristic(true, "constraints.stepVal", { 0, format[fmt].max }, fmt, getDftNumberConstraints(fmt))
    testCharacteristic(false, "constraints.stepVal", { -1, format[fmt].max + 1, "test", {}, false }, fmt, getDftNumberConstraints(fmt))
end

---Initialize with valid constraints validVals.
testCharacteristic(true, "constraints.validVals", { { format.UInt8.min, format.UInt8.max } }, "UInt8")

---Initialize with invalid constraints validVals.
testCharacteristic(false, "constraints.validVals", { { format.UInt8.min - 1, format.UInt8.max + 1 }, { true, "test" }, "test", true, 1 }, "UInt8")
testCharacteristic(false, "constraints.validVals", { { format.UInt8.min, format.UInt8.max } }, "Bool")

---Initialize with valid constraints validValsRanges.
testCharacteristic(true, "constraints.validValsRanges", { { { start = format.UInt8.min, stop = format.UInt8.max } } }, "UInt8")

---Initialize with mutli constraints validValsRanges.
testCharacteristic(true, "constraints.validValsRanges", { { { start = 1, stop = 2 }, { start = 4, stop = 5 } } }, "UInt8")

---Initialize with invalid constraints validValsRanges.
testCharacteristic(false, "constraints.validValsRanges", { false, "test", 1, { false }, { "test" }, { start = false }, { { start = format.UInt8.min, stop = true } } }, "UInt8")
testCharacteristic(false, "constraints.validValsRanges", { { { start = format.UInt8.min, stop = format.UInt8.max } } }, "Bool")
