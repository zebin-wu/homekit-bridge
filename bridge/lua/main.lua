local config = require("config")
local plugin = require("plugin")
local accessory = require("hap.accessory")
local logger = log.getLogger()

return hap.configure({
    aid = accessory.iid(),
    category = "Bridges",
    name = config.bridge.name or "HomeKit Bridge",
    mfg = pal.board.getManufacturer(),
    model = pal.board.getModel(),
    sn = pal.board.getSerialNumber(),
    firmwareVersion = pal.board.getFirmwareVersion(),
    hardwareVersion = pal.board.getHardwareVersion(),
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
