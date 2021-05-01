local config = require("config")
local plugin = require("plugin")

local logger = log.getLogger()

return hap.configure({
    aid = 1, -- Primary accessory must have aid 1.
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
        identify = function (request)
            logger:info("Identify callback is called.")
            logger:info(string.format("transportType: %s, remote: %s.",
                request.transportType, request.remote))
            return hap.Error.None
        end
    }
}, plugin.gen(config.accessiries), {
    updatedState = function (state)
        logger:default(string.format("Accessory Server State did update: %s.", state))
    end,
    sessionAccept = function ()
        logger:default("Session is accepted")
    end,
    sessionInvalidate = function ()
        logger:default("Session is invalidated")
    end
}, false)
