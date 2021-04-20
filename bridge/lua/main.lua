local config = require("config")
local pfm = require("pfm")
local plugin = require("plugin")
local hap = require("hap")

return hap.configure({
    aid = 1,
    category = "Bridges",
    name = config.bridge.name,
    manufacturer = pfm.getManufacturer(),
    model = pfm.getModel(),
    serialNumber = pfm.getSerialNumber(),
    firmwareVersion = pfm.getFirmwareVersion(),
    hardwareVersion = pfm.getHardwareVersion(),
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
