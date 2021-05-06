local config = require("config")
local core = require("core")

local logger = log.getLogger()
local board = pal.board

return hap.configure({
    aid = 1, -- Primary accessory must have aid 1.
    category = "Bridges",
    name = config.bridge.name or "HomeKit Bridge",
    mfg = board.getManufacturer(),
    model = board.getModel(),
    sn = board.getSerialNumber(),
    fwVer = board.getFirmwareVersion(),
    hwVer = board.getHardwareVersion(),
    services = {
        hap.AccessoryInformationService,
        hap.HapProtocolInformationService,
        hap.PairingService,
    },
    cbs = {
        identify = function (request)
            logger:info("Identify callback is called.")
            logger:info(string.format("transportType: %s, remote: %s.",
                request.transportType, request.remote))
            return hap.Error.None
        end
    }
}, core.gen(config.accessories), {
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
