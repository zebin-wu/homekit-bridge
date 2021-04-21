local config = require("config")
local pal = require("pal")
local plugin = require("plugin")
local hap = require("hap")

return hap.configure({
    aid = 1,
    category = "Bridges",
    name = config.bridge.name,
    manufacturer = pal.getManufacturer(),
    model = pal.getModel(),
    serialNumber = pal.getSerialNumber(),
    firmwareVersion = pal.getFirmwareVersion(),
    hardwareVersion = pal.getHardwareVersion(),
    services = {
        hap.AccessoryInformationService,
        hap.HapProtocolInformationService,
        hap.PairingService,
    },
    callbacks = {
        identify = function(request)
            print("Identify callback is called.")
            print(string.format("transportType: %s, remote: %s.",
                request.transportType, request.remote))
            return hap.Error.None
        end
    }
}, plugin.gen(config.accessiries))
