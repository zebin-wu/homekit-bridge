---@meta

---@class Accessory:table HomeKit accessory.
---
---@field aid integer Accessory instance ID.
---@field category integer Category information for the accessory.
---@field name string The display name of the accessory.
---@field manufacturer string The manufacturer of the accessory.
---@field model string The model name of the accessory.
---@field serialNumber string The serial number of the accessory.
---@field firmwareVersion string The firmware version of the accessory.
---@field hardwareVersion string The hardware version of the accessory.
---@field services array Array of provided services.
---@field callbacks AccessoryCallbacks Callbacks.

---@class AccessoryCallbacks:table Accessory Callbacks.
---
---@field identify function The callback used to invoke the identify routine.

---@class Service:table HomeKit service.
---
---@field iid integer Instance ID.
---@field serviceType integer The type of the service.
---@field debugDescription string Description for debugging (based on "Type" field of HAP specification).
---@field name string The name of the service.
---@field properties ServiceProperties HAP Service properties.
---@field linkedServices array Array containing instance IDs of linked services.
---@field characteristics array Array of contained characteristics.

---@class ServiceProperties:table Properties that HomeKit services can have.
---
---@field primaryService boolean The service is the primary service on the accessory.
---@field hidden boolean The service should be hidden from the user.
---@field ble ServicePropertiesBLE The service supports configuration.

---@class ServicePropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsConfiguration boolean The service supports configuration.

---@class array: table

---Error type.
local Error = {
    None = 0,
    Unknown = 1,
    InvalidState = 2,
    InvalidData = 3,
    OutOfResources = 4,
    NotAuthorized = 5,
    Busy = 6,
}

---Accessory category.
local AccessoryCategory = {
    BridgedAccessory = 0,
    Other = 1,
    Bridges = 2,
    Fans = 3,
    GarageDoorOpeners = 4,
    Lighting = 5,
    Locks = 6,
    Outlets = 7,
    Switches = 8,
    Thermostats = 9,
    Sensors = 10,
    SecuritySystems = 11,
    Doors = 12,
    Windows = 13,
    WindowCoverings = 14,
    ProgrammableSwitches = 15,
    RangeExtenders = 16,
    IPCameras = 17,
    AirPurifiers = 19,
    Heaters = 20,
    AirConditioners = 21,
    Humidifiers = 22,
    Dehumidifiers = 23,
    Sprinklers = 28,
    Faucets = 29,
    ShowerSystems = 30
}

---Service type.
local ServiceType = {
    AccessoryInformation = 0,
    GarageDoorOpener = 1,
    LightBulb = 2,
    Lightbulb = 3,
    LockManagement = 4,
    Outlet = 5,
    Switch = 6,
    Thermostat = 7,
    Pairing = 8,
    SecuritySystem = 9,
    CarbonMonoxideSensor = 10,
    ContactSensor = 11,
    Door = 12,
    HumiditySensor = 13,
    LeakSensor = 14,
    LightSensor = 15,
    MotionSensor = 16,
    OccupancySensor = 17,
    SmokeSensor = 18,
    StatelessProgrammableSwitch = 19,
    TemperatureSensor = 20,
    Window = 21,
    WindowCovering = 22,
    AirQualitySensor = 23,
    BatteryService = 24,
    CarbonDioxideSensor = 25,
    HAPProtocolInformation = 26,
    Fan = 27,
    FanV2 = 28,
    Slat = 29,
    FilterMaintenance = 30,
    AirPurifier = 31,
    HeaterCooler = 32,
    HumidifierDehumidifier = 33,
    ServiceLabel = 34,
    IrrigationSystem = 35,
    Valve = 36,
    Faucet = 37,
    CameraRTPStreamManagement = 38,
    Microphone = 39,
    Speaker = 40,
}

---Configure HAP.
---@param accessory Accessory Accessory to serve.
---@param bridgedAccessories? array Array of bridged accessories.
---@return boolean
local function configure(accessory, bridgedAccessories) end

---HomeKit Accessory Information service.
---@type lightuserdata
local AccessoryInformationService

---HAP Protocol Information service.
---@type lightuserdata
local HapProtocolInformationService

---Pairing service.
---@type lightuserdata
local PairingService

return {
    configure = configure,
    Error = Error,
    AccessoryCategory = AccessoryCategory,
    ServiceType = ServiceType,
    AccessoryInformationService = AccessoryInformationService,
    HapProtocolInformationService = HapProtocolInformationService,
    PairingService = PairingService,
}
