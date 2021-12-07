---@class haplib
local hap = {}

---@class HapSession:lightuserdata HomeKit Session.

---@class HapServerCallbacks:table Accessory server callbacks.
---
---@field updatedState async fun(state: HapServerState) Invoked when the accessory server state changes.
---@field sessionAccept async fun(session: HapSession) The callback used when a HomeKit Session is accepted.
---@field sessionInvalidate async fun(session: HapSession) The callback used when a HomeKit Session is invalidated.

---@class HapAccessory:table HomeKit accessory.
---
---@field aid integer Accessory instance ID.
---@field category HapAccessoryCategory Category information for the accessory.
---@field name string The display name of the accessory.
---@field mfg string The manufacturer of the accessory.
---@field model string The model name of the accessory.
---@field sn string The serial number of the accessory.
---@field fwVer string The firmware version of the accessory.
---@field hwVer string The hardware version of the accessory.
---@field services HapService[] Array of provided services.
---@field cbs HapAccessoryCallbacks Callbacks.
---@field context any Accessory context. Will be passed to callbacks.

---@class HapAccessoryCallbacks:table Accessory Callbacks.
---
---@field identify fun(request: HapAccessoryIdentifyRequest, context?: any):HapError The callback used to invoke the identify routine.

---@class HapAccessoryInformation:table Accessory information.
---
---@field aid integer Accessory instance ID.
---@field category HapAccessoryCategory Category information for the accessory.
---@field name string The display name of the accessory.

---@class HapAccessoryIdentifyRequest:table Accessory identify request.
---
---@field transportType HapTransportType Transport type over which the request has been received.
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.
---@field session HapSession The session over which the request has been received.
---@field accessory HapAccessoryInformation

---@class HapService:table HomeKit service.
---
---@field iid integer Instance ID.
---@field type HapServiceType The type of the service.
---@field name string The name of the service.
---@field props HapServiceProperties HAP Service properties.
---@field linkedServices integer[] Array containing instance IDs of linked services.
---@field chars HapCharacteristic[] Array of contained characteristics.

---@class HapServiceProperties:table Properties that HomeKit services can have.
---
---@field primaryService boolean The service is the primary service on the accessory.
---@field hidden boolean The service should be hidden from the user.
---@field ble HapServicePropertiesBLE

---@class HapServicePropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsConfiguration boolean The service supports configuration. Only the HAP Protocol Information service may support configuration.

---@class HapServiceInformation
---
---@field iid integer Instance ID.
---@field type HapServiceType The type of the service.
---@field name string The name of the service.

---@class HapCharacteristic:table HomeKit characteristic.
---
---@field format HapCharacteristicFormat Format.
---@field iid integer Instance ID.
---@field type HapCharacteristicType The type of the characteristic.
---@field mfgDesc string Description of the characteristic provided by the manufacturer of the accessory.
---@field props HapCharacteristicProperties Characteristic properties.
---@field units HapCharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
---@field constraints HapStringCharacteristiConstraints|HapNumberCharacteristiConstraints|HapUInt8CharacteristiConstraints Value constraints.
---@field cbs HapCharacteristicCallbacks Callbacks.

---@class HapCharacteristicInformation
---
---@field iid integer Instance ID.
---@field format HapCharacteristicFormat Format.
---@field type HapCharacteristicType The type of the characteristic.

---@class HapStringCharacteristiConstraints:table Format: String|Data
---
---@field maxLen integer Maximum length.

---@class HapNumberCharacteristiConstraints:table Format: UInt16|UInt32|Uint64|Int|Float
---
---@field minVal number Minimum value.
---@field maxVal number Maximum value.
---@field stepVal number Step value.

---@class HapUInt8CharacteristiConstraints:table Format: UInt8
---
---@field minVal integer Minimum value.
---@field maxVal integer Maximum value.
---@field stepVal integer Step value.
---
---Only supported for Apple defined characteristics.
---@field validVals integer[] List of valid values in ascending order.
---@field validValsRanges HapUInt8CharacteristicValidValuesRange[] List of valid values ranges in ascending order.

---@class HapUInt8CharacteristicValidValuesRange
---
---@field start integer Starting value.
---@field stop integer Ending value.

---@class HapCharacteristicReadRequest:table Characteristic read request.
---
---@field transportType HapTransportType Transport type over which the request has been received.
---@field session HapSession The session over which the request has been received.
---@field accessory HapAccessoryInformation
---@field service HapServiceInformation
---@field characteristic HapCharacteristicInformation

---@class HapCharacteristicWriteRequest:table Characteristic write request.
---
---@field transportType HapTransportType Transport type over which the request has been received.
---@field session HapSession The session over which the request has been received.
---@field accessory HapAccessoryInformation
---@field service HapServiceInformation
---@field characteristic HapCharacteristicInformation
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.

---@class HapCharacteristicSubscriptionRequest:table Characteristic subscription request.
---
---@field transportType HapTransportType Transport type over which the request has been received.
---@field session HapSession The session over which the request has been received.
---@field accessory HapAccessoryInformation
---@field service HapServiceInformation
---@field characteristic HapCharacteristicInformation

---@class HapCharacteristicCallbacks:table Characteristic Callbacks.
---
---@field read async fun(request:HapCharacteristicReadRequest, context?:any): any, HapError The callback used to handle read requests, it returns value and error.
---@field write async fun(request:HapCharacteristicWriteRequest, value:any, context?:any): HapError The callback used to handle write requests, it return error.
---@field sub async fun(request:HapCharacteristicSubscriptionRequest, context?:any) The callback used to handle subscribe requests.
---@field unsub async fun(request:HapCharacteristicSubscriptionRequest, context?:any) The callback used to handle unsubscribe requests.

---@class HapCharacteristicProperties:table Properties that HomeKit characteristics can have.
---
---@field readable boolean The characteristic is readable.
---@field writable boolean The characteristic is writable.
---@field supportsEventNotification boolean The characteristic supports notifications using the event connection established by the controller.
---@field hidden boolean The characteristic should be hidden from the user.
---@field readRequiresAdminPermissions boolean The characteristic will only be accessible for read operations by admin controllers.
---@field writeRequiresAdminPermissions boolean The characteristic will only be accessible for write operations by admin controllers.
---@field requiresTimedWrite boolean The characteristic requires time sensitive actions.
---@field supportsAuthorizationData boolean The characteristic requires additional authorization data.
---@field ip HapCharacteristicPropertiesIP
---@field ble HapCharacteristicPropertiesBLE

---@class HapCharacteristicPropertiesIP:table These properties only affect connections over IP (Ethernet / Wi-Fi).
---
---@field controlPoint boolean This flag prevents the characteristic from being read during discovery.
---@field supportsWriteResponse boolean Write operations on the characteristic require a read response value.

---@class HapCharacteristicPropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsBroadcastNotification boolean The characteristic supports broadcast notifications.
---@field supportsDisconnectedNotification boolean The characteristic supports disconnected notifications.
---@field readableWithoutSecurity boolean The characteristic is always readable, even before a secured session is established.
---@field writableWithoutSecurity boolean The characteristic is always writable, even before a secured session is established.

---@alias HapServerState
---| '"Idle"'
---| '"Running"'
---| '"Stopping"'

---@alias HapTransportType
---| '"IP"'     # HAP over IP (Ethernet / Wi-Fi).
---| '"BLE"'    # HAP over Bluetooth LE.

---@alias HapAccessoryCategory
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

---@alias HapServiceType
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

---@alias HapCharacteristicFormat
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

---@alias HapCharacteristicType
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

---@alias HapCharacteristicUnits
---| '"None"'       # Unitless. Used for example on enumerations.
---| '"Celsius"'    # Degrees celsius.
---| '"ArcDegrees"' # The degrees of an arc.
---| '"Percentage"' # A percentage %.
---| '"Lux"'        # Lux (that is, illuminance).
---| '"Seconds"'    # Seconds.

---@class HapError:integer Error type.

---``ENUM`` Error type.
hap.Error = {
    None = 0,               ---No error occurred.
    Unknown = 1,            ---Unknown error.
    InvalidState = 2,       ---Operation is not supported in current state.
    InvalidData = 3,        ---Data has unexpected format.
    OutOfResources = 4,     ---Out of resources.
    NotAuthorized = 5,      ---Insufficient authorization.
    Busy = 6,               ---Operation failed temporarily, retry later.
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

---Initialize HAP.
---@param primaryAccessory HapAccessory Primary accessory to serve.
---@param bridgedAccessories HapAccessory[] Array of bridged accessories.
---@param serverCallbacks HapServerCallbacks Accessory server callbacks.
---@return boolean status true on success, false on failure.
function hap.init(primaryAccessory, bridgedAccessories, serverCallbacks) end

---De-initialize then you can init() again.
function hap.deinit() end

---Start accessory server, you must init() first.
---@param confChanged boolean Whether or not the bridge configuration changed since the last start.
function hap.start(confChanged) end

---Stop accessory server.
function hap.stop() end

---Raises an event notification for a given characteristic in a given service provided by a given accessory.
---If has session, it raises event on a given session.
---@overload fun(accessoryIID: integer, serviceIID: integer, characteristicIID: integer)
---@param accessoryIID integer Accessory instance ID.
---@param serviceIID integer Service instance ID.
---@param characteristicIID integer characteristic intstance ID.
---@param session? HapSession The session on which to raise the event.
function hap.raiseEvent(accessoryIID, serviceIID, characteristicIID, session) end

---Get a new Instance ID for bridged accessory.
---@return integer iid Instance ID.
function hap.getNewBridgedAccessoryID() end

---Get a new Instance ID for service or characteristic.
---@return integer iid Instance ID.
function hap.getNewInstanceID() end

return hap
