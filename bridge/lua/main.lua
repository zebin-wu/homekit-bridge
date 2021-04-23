local config = require("config")
local plugin = require("plugin")
local hap = require("hap")
local board = require("pal").board
local logger = require("log").getLogger()

return hap.configure({
    aid = 1,
    category = "Bridges",
    name = config.bridge.name,
    manufacturer = board.getManufacturer(),
    model = board.getModel(),
    serialNumber = board.getSerialNumber(),
    firmwareVersion = board.getFirmwareVersion(),
    hardwareVersion = board.getHardwareVersion(),
    services = {
        hap.AccessoryInformationService,
        hap.HapProtocolInformationService,
        hap.PairingService,
    },
    callbacks = {
        identify = function(request)
            logger:info("Identify callback is called.")
            logger:info(string.format("transportType: %s, remote: %s.",
                request.transportType, request.remote))
            return hap.Error.None
        end
    }
}, plugin.gen(config.accessiries))
