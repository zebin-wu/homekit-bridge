local config = require("config")
local pfm = require("pfm")
local plugin = require("plugin")
local hap = require("hap")

hap.start({
    aid = 1,
    category = hap.AccessoryCategory.Bridges,
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
        identify = function()
            print("Identify callback is called.")
            return hap.Error.None
        end
    }
},
plugin.gen(config.accessiries),
true)
