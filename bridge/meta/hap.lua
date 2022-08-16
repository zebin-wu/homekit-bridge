---@meta

---@class haplib
local M = {}

---@class HAPSession:lightuserdata HomeKit Session.

---@class HAPAccessory:userdata HomeKit accessory.

---@class HAPService:userdata HomeKit service.
local service = {}

---Link services.
---@param ... integer Instance IDs of linked services.
---@return HAPService self
function service:linkServices(...) end

---@class HAPCharacteristic:userdata HomeKit characteristic.
local characteristic = {}

---Set the manufacturer description for the characteristic.
---@param mfgDesc? string Description of the characteristic provided by the manufacturer of the accessory.
---@return HAPCharacteristic self
function characteristic:setMfgDesc(mfgDesc) end

---Set units for characteristic.
---@param units HAPCharacteristicUnits The units of the values for the characteristic. Format: UInt8|UInt16|UInt32|UInt64|Int|Float
---@return HAPCharacteristic self
function characteristic:setUnits(units) end

---Set contraints for characteristic.
---@overload fun(characteristic: HAPCharacteristic, maxLen: integer): HAPCharacteristic
---@param minVal number Minimum value.
---@param maxVal number Maximum value.
---@param stepVal number Step value.
---@return HAPCharacteristic self
function characteristic:setContraints(minVal, maxVal, stepVal) end

---Set valid values for ``UInt8`` characteristic. Only supported for Apple defined characteristics.
---@param ... integer Valid values in ascending order.
---@return HAPCharacteristic self
function characteristic:setValidVals(...) end

---Valid values ranges, is an array of length 2.
---Element 1 represents the ``start`` value.
---Element 2 represents the ``end`` value.
---@class HAPValidValsRanges:integer[]

---Set valid values ranges for ``UInt8`` characteristic. Only supported for Apple defined characteristics.
---@param ... HAPValidValsRanges Valid values ranges in ascending order.
---@return HAPCharacteristic self
function characteristic:setValidValsRanges(...) end

---@class HAPAccessoryIdentifyRequest:table Accessory identify request.
---
---@field transportType HAPTransportType Transport type over which the request has been received.
---@field remote boolean Whether the request appears to have originated from a remote controller, e.g. via Apple TV.
---@field session HAPSession The session over which the request has been received.
---@field aid integer Accessory instance ID.

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
---@type HAPService
M.AccessoryInformationService = {}

---HAP Protocol Information service.
---@type HAPService
M.HAPProtocolInformationService = {}

---Pairing service.
---@type HAPService
M.PairingService = {}

---New a accessory.
---@param aid integer Accessory instance ID.
---@param category HAPAccessoryCategory Category information for the accessory.
---@param name string The display name of the accessory.
---@param manufacturer string The manufacturer of the accessory.
---@param model string The model name of the accessory.
---@param serialNumber string The serial number of the accessory.
---@param firmwareVersion string The firmware version of the accessory.
---@param hardwareVersion string The hardware version of the accessory.
---@param services HAPService[] The services of the accessory.
---@param identify? async fun(request: HAPAccessoryIdentifyRequest) The callback used to invoke the identify routine.
---@return HAPAccessory
function M.newAccessory(aid, category, name, manufacturer, model, serialNumber, firmwareVersion, hardwareVersion, services, identify) end

---New a service.
---@param iid integer Instance ID.
---@param type HAPServiceType The type of the service.
---@param primary boolean The service is the primary service on the accessory.
---@param hidden boolean The service should be hidden from the user.
---@param characteristics HAPCharacteristic[] The characteristics of the service.
---@return HAPService
function M.newService(iid, type, primary, hidden, characteristics) end

---New a characteristic.
---@param iid integer Instance ID.
---@param format HAPCharacteristicFormat Characteristic format.
---@param type HAPCharacteristicType The type of the characteristic.
---@param props HAPCharacteristicProperties Characteristic properties.
---@param read? async fun(request: HAPCharacteristicReadRequest): any The callback used to handle read requests, it returns value.
---@param write? async fun(request: HAPCharacteristicWriteRequest, value: any) The callback used to handle write requests.
---@param sub? async fun(request: HAPCharacteristicSubscriptionRequest) The callback used to handle subscribe requests.
---@param unsub? async fun(request: HAPCharacteristicSubscriptionRequest) The callback used to handle unsubscribe requests.
---@return HAPCharacteristic
function M.newCharacteristic(iid, format, type, props, read, write, sub, unsub) end

---Accessory is valid.
---@param accessory HAPAccessory HAP accessory.
---@param bridged? boolean Whether the accessory is bridged accessory.
function M.accessoryIsValid(accessory, bridged) end

---Start accessory server.
---@param primaryAccessory HAPAccessory Primary accessory to serve.
---@param bridgedAccessories? HAPAccessory[] Bridged accessories.
---@param confChanged boolean Whether or not the bridge configuration changed since the last start.
---@param sessionAccept? async fun(session: HAPSession) The callback used when a HomeKit Session is accepted.
---@param sessionInvalidate? async fun(session: HAPSession) The callback used when a HomeKit Session is invalidated.
function M.start(primaryAccessory, bridgedAccessories, confChanged, sessionAccept, sessionInvalidate) end

---Stop accessory server.
function M.stop() end

---Raises an event notification for a given characteristic in a given service provided by a given accessory.
---If has session, it raises event on a given session.
---@overload fun(aid: integer, sid: integer, cid: integer)
---@param aid integer Accessory instance ID.
---@param sid integer Service instance ID.
---@param cid integer Characteristic intstance ID.
---@param session? HAPSession The session on which to raise the event.
function M.raiseEvent(aid, sid, cid, session) end

---Get a new Instance ID for bridged accessory or service or characteristic.
---@param bridgedAccessory? boolean Whether or not to get new IID for bridged accessory.
---@return integer iid Instance ID.
---@nodiscard
function M.getNewInstanceID(bridgedAccessory) end

---Restore factory settings.
---
---This function must be called before calling start().
function M.restoreFactorySettings() end

return M
