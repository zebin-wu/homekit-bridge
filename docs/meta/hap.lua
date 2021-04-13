---@meta

---@class Accessory:table HomeKit accessory.
---
---@field aid integer Accessory instance ID.
---@field category AccessoryCategory Category information for the accessory.
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
---@field type ServiceType The type of the service.
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

---@alias AccessoryCategory
---|>'"BridgedAccessory"'
---| '"Other"'
---| '"Bridges"'
---| '"Fans"'
---| '"GarageDoorOpeners"'
---| '"Lighting"'
---| '"Locks"'
---| '"Outlets"'
---| '"Switches"'
---| '"Thermostats"'
---| '"Sensors"'
---| '"SecuritySystems"'
---| '"Doors"'
---| '"Windows"'
---| '"WindowCoverings"'
---| '"ProgrammableSwitches"'
---| '"RangeExtenders"'
---| '"IPCameras"'
---| '"AirPurifiers"'
---| '"Heaters = 20"'
---| '"AirConditioners"'
---| '"Humidifiers"'
---| '"Dehumidifiers"'
---| '"Sprinklers"'
---| '"Faucets"'
---| '"ShowerSystems"'

---@alias ServiceType
---| '"AccessoryInformation"'
---| '"GarageDoorOpener"'
---| '"LightBulb"'
---| '"LockManagement"'
---| '"Outlet"'
---| '"Switch"'
---| '"Thermostat"'
---| '"Pairing"'
---| '"SecuritySystem"'
---| '"CarbonMonoxideSensor"'
---| '"ContactSensor"'
---| '"Door"'
---| '"HumiditySensor"'
---| '"LeakSensor"'
---| '"LightSensor"'
---| '"MotionSensor"'
---| '"OccupancySensor"'
---| '"SmokeSensor"'
---| '"StatelessProgrammableSwitch"'
---| '"TemperatureSensor"'
---| '"Window"'
---| '"WindowCovering"'
---| '"AirQualitySensor"'
---| '"BatteryService"'
---| '"CarbonDioxideSensor"'
---| '"HAPProtocolInformation"'
---| '"Fan"'
---| '"Slat"'
---| '"FilterMaintenance"'
---| '"AirPurifier"'
---| '"HeaterCooler"'
---| '"HumidifierDehumidifier"'
---| '"ServiceLabel"'
---| '"IrrigationSystem"'
---| '"Valve"'
---| '"Faucet"'
---| '"CameraRTPStreamManagement"'
---| '"Microphone"'
---| '"Speaker"'

---HomeKit Accessory Information service.
---@type lightuserdata
local AccessoryInformationService

---HAP Protocol Information service.
---@type lightuserdata
local HapProtocolInformationService

---Pairing service.
---@type lightuserdata
local PairingService

---Configure HAP.
---@param accessory Accessory Accessory to serve.
---@param bridgedAccessories? array Array of bridged accessories.
---@return boolean
local function configure(accessory, bridgedAccessories) end

return {
    Error = Error,
    AccessoryInformationService = AccessoryInformationService,
    HapProtocolInformationService = HapProtocolInformationService,
    PairingService = PairingService,
    configure = configure,
}
