---@meta

---@class haplib
hap = {}

---@class Session:lightuserdata HomeKit Session.

---@class ServerCallbacks:table Accessory server callbacks.
---
---@field updatedState fun(state: ServerState) Invoked when the accessory server state changes.
---@field sessionAccept fun(session: Session) The callback used when a HomeKit Session is accepted.
---@field sessionInvalidate fun(session: Session) The callback used when a HomeKit Session is invalidated.

---@class Accessory:table HomeKit accessory.
---
---@field aid integer Accessory instance ID.
---@field category AccessoryCategory Category information for the accessory.
---@field name string The display name of the accessory.
---@field mfg string The manufacturer of the accessory.
---@field model string The model name of the accessory.
---@field sn string The serial number of the accessory.
---@field fwVer string The firmware version of the accessory.
---@field hwVer string The hardware version of the accessory.
---@field services Service[] Array of provided services.
---@field cbs AccessoryCallbacks Callbacks.
---@field context any Accessory context. Will be passed to callbacks.

---@class AccessoryCallbacks:table Accessory Callbacks.
---
---@field identify fun(request: AccessoryIdentifyRequest, context?:any):Error The callback used to invoke the identify routine.

---@class AccessoryInformation:table Accessory information.
---
---@field aid integer Accessory instance ID.
---@field category AccessoryCategory Category information for the accessory.
---@field name string The display name of the accessory.

---@class AccessoryIdentifyRequest:table Accessory identify request.
---
---@field transportType TransportType Transport type over which the request has been received.
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.
---@field session Session The session over which the request has been received.
---@field accessory AccessoryInformation

---@class Service:table HomeKit service.
---
---@field iid integer Instance ID.
---@field type ServiceType The type of the service.
---@field name string The name of the service.
---@field props ServiceProperties HAP Service properties.
---@field linkedServices integer[] Array containing instance IDs of linked services.
---@field chars Characteristic[] Array of contained characteristics.

---@class ServiceProperties:table Properties that HomeKit services can have.
---
---@field primaryService boolean The service is the primary service on the accessory.
---@field hidden boolean The service should be hidden from the user.
---@field ble ServicePropertiesBLE

---@class ServicePropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsConfiguration boolean The service supports configuration.

---@class ServiceInformation
---
---@field iid integer Instance ID.
---@field type ServiceType The type of the service.
---@field name string The name of the service.

---@class Characteristic:table HomeKit characteristic.
---
---@field format CharacteristicFormat Format.
---@field iid integer Instance ID.
---@field type CharacteristicType The type of the characteristic.
---@field mfgDesc string Description of the characteristic provided by the manufacturer of the accessory.
---@field props CharacteristicProperties Characteristic properties.
---@field units CharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
---@field constraints StringCharacteristiConstraints|NumberCharacteristiConstraints|UInt8CharacteristiConstraints Value constraints.
---@field cbs CharacteristicCallbacks Callbacks.

---@class CharacteristicInformation
---
---@field iid integer Instance ID.
---@field format CharacteristicFormat Format.
---@field type CharacteristicType The type of the characteristic.

---@class StringCharacteristiConstraints:table Format: String|Data
---
---@field maxLen integer Maximum length.

---@class NumberCharacteristiConstraints:table Format: UInt16|UInt32|Uint64|Int|Float
---
---@field minVal number Minimum value.
---@field maxVal number Maximum value.
---@field stepVal number Step value.

---@class UInt8CharacteristiConstraints:table Format: UInt8
---
---@field minVal integer Minimum value.
---@field maxVal integer Maximum value.
---@field stepVal integer Step value.
---@field validVals integer[] List of valid values in ascending order.
---@field validValsRanges integer[] List of valid values ranges in ascending order.

---@class UInt8CharacteristicValidValuesRange
---
---@field start integer Starting value.
---@field end integer Starting value.

---@class CharacteristicReadRequest:table Characteristic read request.
---
---@field transportType TransportType Transport type over which the request has been received.
---@field session Session The session over which the request has been received.
---@field accessory AccessoryInformation
---@field service ServiceInformation
---@field characteristic CharacteristicInformation

---@class CharacteristicWriteRequest:table Characteristic write request.
---
---@field transportType TransportType Transport type over which the request has been received.
---@field session Session The session over which the request has been received.
---@field accessory AccessoryInformation
---@field service ServiceInformation
---@field characteristic CharacteristicInformation
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.

---@class CharacteristicSubscriptionRequest:table Characteristic subscription request.
---
---@field transportType TransportType Transport type over which the request has been received.
---@field session Session The session over which the request has been received.
---@field accessory AccessoryInformation
---@field service ServiceInformation
---@field characteristic CharacteristicInformation

---@class CharacteristicCallbacks:table Characteristic Callbacks.
---
---@field read fun(request:CharacteristicReadRequest, context?:any):any, Error The callback used to handle read requests, it returns value and error.
---@field write fun(request:CharacteristicWriteRequest, val:any, context?:any):boolean, Error The callback used to handle write requests, it return changed flag and error.
---@field sub fun(request:CharacteristicSubscriptionRequest, context?:any) The callback used to handle subscribe requests.
---@field unsub fun(request:CharacteristicSubscriptionRequest, context?:any) The callback used to handle unsubscribe requests.

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

---@alias ServerState
---| '"Idle"'
---| '"Running"'
---| '"Stopping"'

---@alias TransportType
---| '"IP"'     # HAP over IP (Ethernet / Wi-Fi).
---| '"BLE"'    # HAP over Bluetooth LE.

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
---| '"Heaters"'
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
---| '"LockMechanism"'
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

---@class Error:integer Error type.

---``ENUM`` Error type.
hap.Error = {
    None = 0,
    Unknown = 1,
    InvalidState = 2,
    InvalidData = 3,
    OutOfResources = 4,
    NotAuthorized = 5,
    Busy = 6,
}

---HomeKit Accessory Information service.
---@type lightuserdata
hap.AccessoryInformationService = {}

---HAP Protocol Information service.
---@type lightuserdata
hap.HapProtocolInformationService = {}

---Pairing service.
---@type lightuserdata
hap.PairingService = {}

---Configure HAP.
---@param primaryAccessory Accessory Primary accessory to serve.
---@param bridgedAccessories Accessory[] Array of bridged accessories.
---@param serverCallbacks ServerCallbacks Accessory server callbacks.
---@param confChanged boolean Whether or not the bridge configuration changed since the last start.
---@return boolean status true on success, false on failure.
function hap.configure(primaryAccessory, bridgedAccessories, serverCallbacks, confChanged) end

---Raises an event notification for a given characteristic in a given service provided by a given accessory.
---If has session, it raises event on a given session.
---@overload fun(accessoryIID: integer, serviceIID: integer, characteristicIID: integer)
---@param accessoryIID integer Accessory instance ID.
---@param serviceIID integer Service instance ID.
---@param characteristicIID integer characteristic intstance ID.
---@param session? Session The session on which to raise the event.
function hap.raiseEvent(accessoryIID, serviceIID, characteristicIID, session) end

---Unconfigure all accessires then you can configure() again.
function hap.unconfigure() end

---Get a new Instance ID for bridged accessory.
---@return integer iid Instance ID.
function hap.getNewBridgedAccessoryID() end

---Get a new Instance ID for service or characteristic.
---@return integer iid Instance ID.
function hap.getNewInstanceID() end

return hap
