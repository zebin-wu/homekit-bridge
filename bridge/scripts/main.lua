local config = require "config"
local plugins = require "plugins"
local hap = require "hap"
local chip = require "chip"

local logger = log.getLogger()

hap.init({
    aid = 1, -- Primary accessory must have aid 1.
    category = "Bridges",
    name = config.bridge.name or "HomeKit Bridge",
    mfg = chip.getInfo("mfg"),
    model = chip.getInfo("model"),
    sn = chip.getInfo("sn"),
    ---@diagnostic disable-next-line: undefined-global
    fwVer = _BRIDGE_VERSION,
    hwVer = chip.getInfo("hwver"),
    services = {
        hap.AccessoryInformationService,
        hap.HAPProtocolInformationService,
        hap.PairingService,
    },
    cbs = {
        identify = function (request)
            logger:info("Identify callback is called.")
        end
    }
}, {
    updatedState = function (state)
        logger:default("Accessory Server State did update: " .. state .. ".")
        plugins.handleState(state)
    end,
    sessionAccept = function (session)
        logger:default(("Session %p is accepted."):format(session))
    end,
    sessionInvalidate = function (session)
        logger:default(("Session %p is invalidated."):format(session))
    end
})

plugins.init(config.plugins)

hap.start(true)
