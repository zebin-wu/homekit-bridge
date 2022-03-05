---@meta

---@class haplib
local hap = {}

---@class HAPSession:lightuserdata HomeKit Session.

---@class HAPServerCallbacks:table Accessory server callbacks.
---
---@field updatedState async fun(state: HAPServerState) Invoked when the accessory server state changes.
---@field sessionAccept async fun(session: HAPSession) The callback used when a HomeKit Session is accepted.
---@field sessionInvalidate async fun(session: HAPSession) The callback used when a HomeKit Session is invalidated.

---@class HAPAccessory:table HomeKit accessory.
---
---@field aid integer Accessory instance ID.
---@field category HAPAccessoryCategory Category information for the accessory.
---@field name string The display name of the accessory.
---@field mfg string The manufacturer of the accessory.
---@field model string The model name of the accessory.
---@field sn string The serial number of the accessory.
---@field fwVer string The firmware version of the accessory.
---@field hwVer string The hardware version of the accessory.
---@field services HAPService[] Array of provided services.
---@field cbs HAPAccessoryCallbacks Callbacks.

---@class HAPAccessoryCallbacks:table Accessory Callbacks.
---
---@field identify fun(request: HAPAccessoryIdentifyRequest) The callback used to invoke the identify routine.

---@class HAPAccessoryIdentifyRequest:table Accessory identify request.
---
---@field transportType HAPTransportType Transport type over which the request has been received.
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.
---@field session HAPSession The session over which the request has been received.
---@field aid integer Accessory instance ID.

---@class HAPService:table HomeKit service.
---
---@field iid integer Instance ID.
---@field type HAPServiceType The type of the service.
---@field props HAPServiceProperties HAP Service properties.
---@field linkedServices integer[] Array containing instance IDs of linked services.
---@field chars HAPCharacteristic[] Array of contained characteristics.

---@class HAPServiceProperties:table Properties that HomeKit services can have.
---
---@field primaryService boolean The service is the primary service on the accessory.
---@field hidden boolean The service should be hidden from the user.
---@field ble HAPServicePropertiesBLE

---@class HAPServicePropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsConfiguration boolean The service supports configuration. Only the HAP Protocol Information service may support configuration.

---@class HAPCharacteristic:table HomeKit characteristic.
---
---@field format HAPCharacteristicFormat Format.
---@field iid integer Instance ID.
---@field type HAPCharacteristicType The type of the characteristic.
---@field mfgDesc string Description of the characteristic provided by the manufacturer of the accessory.
---@field props HAPCharacteristicProperties Characteristic properties.
---@field units HAPCharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
---@field constraints HAPStringCharacteristiConstraints|HAPNumberCharacteristiConstraints|HAPUInt8CharacteristiConstraints Value constraints.
---@field cbs HAPCharacteristicCallbacks Callbacks.

---@class HAPStringCharacteristiConstraints:table Format: String|Data
---
---@field maxLen integer Maximum length.

---@class HAPNumberCharacteristiConstraints:table Format: UInt16|UInt32|Uint64|Int|Float
---
---@field minVal number Minimum value.
---@field maxVal number Maximum value.
---@field stepVal number Step value.

---@class HAPUInt8CharacteristiConstraints:table Format: UInt8
---
---@field minVal integer Minimum value.
---@field maxVal integer Maximum value.
---@field stepVal integer Step value.
---
---Only supported for Apple defined characteristics.
---@field validVals integer[] List of valid values in ascending order.
---@field validValsRanges HAPUInt8CharacteristicValidValuesRange[] List of valid values ranges in ascending order.

---@class HAPUInt8CharacteristicValidValuesRange
---
---@field start integer Starting value.
---@field stop integer Ending value.

---@class HAPCharacteristicReadRequest:table Characteristic read request.
---
---@field transportType HAPTransportType Transport type over which the request has been received.
---@field session HAPSession The session over which the request has been received.
---@field aid integer Accessory instance ID.
---@field sid integer Service instance ID.
---@field cid integer Characteristic intstance ID.

---@class HAPCharacteristicWriteRequest:table Characteristic write request.
---
---@field transportType HAPTransportType Transport type over which the request has been received.
---@field session HAPSession The session over which the request has been received.
---@field aid integer Accessory instance ID.
---@field sid integer Service instance ID.
---@field cid integer Characteristic intstance ID.
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.

---@class HAPCharacteristicSubscriptionRequest:table Characteristic subscription request.
---
---@field transportType HAPTransportType Transport type over which the request has been received.
---@field session HAPSession The session over which the request has been received.
---@field aid integer Accessory instance ID.
---@field sid integer Service instance ID.
---@field cid integer Characteristic intstance ID.

---@class HAPCharacteristicCallbacks:table Characteristic Callbacks.
---
---@field read async fun(request:HAPCharacteristicReadRequest): any The callback used to handle read requests, it returns value.
---@field write async fun(request:HAPCharacteristicWriteRequest, value:any) The callback used to handle write requests.
---@field sub async fun(request:HAPCharacteristicSubscriptionRequest) The callback used to handle subscribe requests.
---@field unsub async fun(request:HAPCharacteristicSubscriptionRequest) The callback used to handle unsubscribe requests.

---@class HAPCharacteristicProperties:table Properties that HomeKit characteristics can have.
---
---@field readable boolean The characteristic is readable.
---@field writable boolean The characteristic is writable.
---@field supportsEventNotification boolean The characteristic supports notifications using the event connection established by the controller.
---@field hidden boolean The characteristic should be hidden from the user.
---@field readRequiresAdminPermissions boolean The characteristic will only be accessible for read operations by admin controllers.
---@field writeRequiresAdminPermissions boolean The characteristic will only be accessible for write operations by admin controllers.
---@field requiresTimedWrite boolean The characteristic requires time sensitive actions.
---@field supportsAuthorizationData boolean The characteristic requires additional authorization data.
---@field ip HAPCharacteristicPropertiesIP
---@field ble HAPCharacteristicPropertiesBLE

---@class HAPCharacteristicPropertiesIP:table These properties only affect connections over IP (Ethernet / Wi-Fi).
---
---@field controlPoint boolean This flag prevents the characteristic from being read during discovery.
---@field supportsWriteResponse boolean Write operations on the characteristic require a read response value.

---@class HAPCharacteristicPropertiesBLE:table These properties only affect connections over Bluetooth LE.
---
---@field supportsBroadcastNotification boolean The characteristic supports broadcast notifications.
---@field supportsDisconnectedNotification boolean The characteristic supports disconnected notifications.
---@field readableWithoutSecurity boolean The characteristic is always readable, even before a secured session is established.
---@field writableWithoutSecurity boolean The characteristic is always writable, even before a secured session is established.

---@alias HAPServerState
---| '"Idle"'
---| '"Running"'
---| '"Stopping"'

---@alias HAPTransportType
---| '"IP"'     # HAP over IP (Ethernet / Wi-Fi).
---| '"BLE"'    # HAP over Bluetooth LE.

---@alias HAPAccessoryCategory
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

---@alias HAPServiceType
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

---@alias HAPCharacteristicFormat
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

---@alias HAPCharacteristicType
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

---@alias HAPCharacteristicUnits
---| '"None"'       # Unitless. Used for example on enumerations.
---| '"Celsius"'    # Degrees celsius.
---| '"ArcDegrees"' # The degrees of an arc.
---| '"Percentage"' # A percentage %.
---| '"Lux"'        # Lux (that is, illuminance).
---| '"Seconds"'    # Seconds.

---HomeKit Accessory Information service.
---@type lightuserdata
hap.AccessoryInformationService = {}

---HAP Protocol Information service.
---@type lightuserdata
hap.HAPProtocolInformationService = {}

---Pairing service.
---@type lightuserdata
hap.PairingService = {}

---Initialize HAP.
---@param primaryAccessory HAPAccessory Primary accessory to serve.
---@param serverCallbacks HAPServerCallbacks Accessory server callbacks.
function hap.init(primaryAccessory, serverCallbacks) end

---De-initialize then you can init() again.
function hap.deinit() end

---Add bridged accessory.
---@param accessory HAPAccessory Bridged accessory.
function hap.addBridgedAccessory(accessory) end

---Start accessory server, you must init() first.
---@param confChanged boolean Whether or not the bridge configuration changed since the last start.
function hap.start(confChanged) end

---Stop accessory server.
function hap.stop() end

---Raises an event notification for a given characteristic in a given service provided by a given accessory.
---If has session, it raises event on a given session.
---@overload fun(aid: integer, sid: integer, cid: integer)
---@param aid integer Accessory instance ID.
---@param sid integer Service instance ID.
---@param cid integer Characteristic intstance ID.
---@param session? HAPSession The session on which to raise the event.
function hap.raiseEvent(aid, sid, cid, session) end

---Get a new Instance ID for bridged accessory or service or characteristic.
---@param bridgedAccessory? boolean Whether or not to get new IID for bridged accessory.
---@return integer iid Instance ID.
---@nodiscard
function hap.getNewInstanceID(bridgedAccessory) end

return hap
