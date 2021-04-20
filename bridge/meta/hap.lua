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

---@class Characteristic:table HomeKit characteristic.
---
---@field format CharacteristicFormat Format.
---@field iid integer Instance ID.
---@field type CharacteristicType The type of the characteristic.
---@field manufacturerDescription string Description of the characteristic provided by the manufacturer of the accessory.
---@field properties CharacteristicProperties Characteristic properties.
---@field units CharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
---@field constraints StringCharacteristiConstraints|NumberCharacteristiConstraints|UInt8CharacteristiConstraints Value constraints.
---@field callbacks CharacteristicCallbacks Callbacks.

---@class StringCharacteristiConstraints:table Format: String|Data
---
---@field maxLength integer Maximum length.

---@class NumberCharacteristiConstraints:table Format: UInt16|UInt32|Uint64|Int|Float
---
---@field minimumValue number Minimum value.
---@field maximumValue number Maximum value.
---@field stepValue number Step value.

---@class UInt8CharacteristiConstraints:table Format: UInt8
---
---@field minimumValue integer Minimum value.
---@field maximumValue integer Maximum value.
---@field stepValue integer Step value.
---@field validValues array List of valid values in ascending order.
---@field validValuesRanges array List of valid values ranges in ascending order.

---@class UInt8CharacteristicValidValuesRange
---
---@field start integer Starting value.
---@field end integer Starting value.

---@class CharacteristicCallbacks:table Characteristic Callbacks.
---
---@field handleRead function The callback used to handle read requests.
---@field handleWrite function The callback used to handle write requests.
---@field handleSubscribe function The callback used to handle subscribe requests.
---@field handleUnsubscribe function The callback used to handle unsubscribe requests.

---@class CharacteristicProperties:table Properties that HomeKit characteristics can have.
---
---@field readable boolean The characteristic is readable.
---@field writable boolean The characteristic is writable.
---@field supportsEventNotification boolean The characteristic supports notifications using the event connection established by the controller.
---@field hidden boolean The characteristic should be hidden from the user.
---@field readRequiresAdminPermissions boolean The characteristic will only be accessible for read operations by admin controllers.
---@field writeRequiresAdminPermissions boolean The characteristic will only be accessible for write operations by admin controllers.
---@field requiresTimedWrite boolean The characteristic requires time sensitive actions.
---@field supportsAuthorizationData boolean The characteristic requires additional authorization data.
---@field ip CharacteristicPropertiesIP
---@field ble CharacteristicPropertiesBLE

---@class CharacteristicPropertiesIP:table These properties only affect connections over IP (Ethernet / Wi-Fi).
---
---@field controlPoint boolean This flag prevents the characteristic from being read during discovery.
---@field supportsWriteResponse boolean Write operations on the characteristic require a read response value.

---@class CharacteristicPropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsBroadcastNotification boolean The characteristic supports broadcast notifications.
---@field supportsDisconnectedNotification boolean The characteristic supports disconnected notifications.
---@field readableWithoutSecurity boolean The characteristic is always readable, even before a secured session is established.
---@field writableWithoutSecurity boolean The characteristic is always writable, even before a secured session is established.

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
---| '"BridgedAccessory"'
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

---@alias CharacteristicFormat
---| '"Data"'
---| '"Bool"'
---| '"UInt8"'
---| '"UInt16"'
---| '"UInt32"'
---| '"UInt64"'
---| '"Int"'
---| '"Float"'
---| '"String"'
---| '"TLV8"'

---@alias CharacteristicType
---| '"AdministratorOnlyAccess"'
---| '"AudioFeedback"'
---| '"Brightness"'
---| '"CoolingThresholdTemperature"'
---| '"CurrentDoorState"'
---| '"CurrentHeatingCoolingState"'
---| '"CurrentRelativeHumidity"'
---| '"CurrentTemperature"'
---| '"HeatingThresholdTemperature"'
---| '"Hue"'
---| '"Identify"'
---| '"LockControlPoint"'
---| '"LockManagementAutoSecurityTimeout"'
---| '"LockLastKnownAction"'
---| '"LockCurrentState"'
---| '"LockTargetState"'
---| '"Logs"'
---| '"Manufacturer"'
---| '"Model"'
---| '"MotionDetected"'
---| '"Name"'
---| '"ObstructionDetected"'
---| '"On"'
---| '"OutletInUse"'
---| '"RotationDirection"'
---| '"RotationSpeed"'
---| '"Saturation"'
---| '"SerialNumber"'
---| '"TargetDoorState"'
---| '"TargetHeatingCoolingState"'
---| '"TargetRelativeHumidity"'
---| '"TargetTemperature"'
---| '"TemperatureDisplayUnits"'
---| '"Version"'
---| '"PairSetup"'
---| '"PairVerify"'
---| '"PairingFeatures"'
---| '"PairingPairings"'
---| '"FirmwareRevision"'
---| '"HardwareRevision"'
---| '"AirParticulateDensity"'
---| '"AirParticulateSize"'
---| '"SecuritySystemCurrentState"'
---| '"SecuritySystemTargetState"'
---| '"BatteryLevel"'
---| '"CarbonMonoxideDetected"'
---| '"ContactSensorState"'
---| '"CurrentAmbientLightLevel"'
---| '"CurrentHorizontalTiltAngle"'
---| '"CurrentPosition"'
---| '"CurrentVerticalTiltAngle"'
---| '"HoldPosition"'
---| '"LeakDetected"'
---| '"OccupancyDetected"'
---| '"PositionState"'
---| '"ProgrammableSwitchEvent"'
---| '"StatusActive"'
---| '"SmokeDetected"'
---| '"StatusFault"'
---| '"StatusJammed"'
---| '"StatusLowBattery"'
---| '"StatusTampered"'
---| '"TargetHorizontalTiltAngle"'
---| '"TargetPosition"'
---| '"TargetVerticalTiltAngle"'
---| '"SecuritySystemAlarmType"'
---| '"ChargingState"'
---| '"CarbonMonoxideLevel"'
---| '"CarbonMonoxidePeakLevel"'
---| '"CarbonDioxideDetected"'
---| '"CarbonDioxideLevel"'
---| '"CarbonDioxidePeakLevel"'
---| '"AirQuality"'
---| '"ServiceSignature"'
---| '"AccessoryFlags"'
---| '"LockPhysicalControls"'
---| '"TargetAirPurifierState"'
---| '"CurrentAirPurifierState"'
---| '"CurrentSlatState"'
---| '"FilterLifeLevel"'
---| '"FilterChangeIndication"'
---| '"ResetFilterIndication"'
---| '"CurrentFanState"'
---| '"Active"'
---| '"CurrentHeaterCoolerState"'
---| '"TargetHeaterCoolerState"'
---| '"CurrentHumidifierDehumidifierState"'
---| '"TargetHumidifierDehumidifierState"'
---| '"WaterLevel"'
---| '"SwingMode"'
---| '"TargetFanState"'
---| '"SlatType"'
---| '"CurrentTiltAngle"'
---| '"TargetTiltAngle"'
---| '"OzoneDensity"'
---| '"NitrogenDioxideDensity"'
---| '"SulphurDioxideDensity"'
---| '"PM2_5Density"'
---| '"PM10Density"'
---| '"VOCDensity"'
---| '"RelativeHumidityDehumidifierThreshold"'
---| '"RelativeHumidityHumidifierThreshold"'
---| '"ServiceLabelIndex"'
---| '"ServiceLabelNamespace"'
---| '"ColorTemperature"'
---| '"ProgramMode"'
---| '"InUse"'
---| '"SetDuration"'
---| '"RemainingDuration"'
---| '"ValveType"'
---| '"IsConfigured"'
---| '"ActiveIdentifier"'
---| '"ADKVersion"'

---@alias CharacteristicUnits
---| '"None"'       # Unitless. Used for example on enumerations.
---| '"Celsius"'    # Degrees celsius.
---| '"ArcDegrees"' # The degrees of an arc.
---| '"Percentage"' # A percentage %.
---| '"Lux"'        # Lux (that is, illuminance).
---| '"Seconds"'    # Seconds.

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
