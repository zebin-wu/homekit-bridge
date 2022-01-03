local config = require "config"
local plugins = require "plugins"
local hap = require "hap"
local chip = require "chip"

local logger = log.getLogger()

plugins.start(config.plugins, config.accessories, function (bridgedAccessories)
    assert(hap.init({
        aid = 1, -- Primary accessory must have aid 1.
        category = "Bridges",
        name = config.bridge.name or "HomeKit Bridge",
        mfg = chip.getInfo("mfg"),
        model = chip.getInfo("model"),
        sn = chip.getInfo("sn"),
        fwVer = _BRIDGE_VERSION,
        hwVer = chip.getInfo("hwver"),
        services = {
            hap.AccessoryInformationService,
            hap.HapProtocolInformationService,
            hap.PairingService,
        },
        cbs = {
            identify = function (request)
                logger:info("Identify callback is called.")
                return hap.Error.None
            end
        }
    }, bridgedAccessories, {
        updatedState = function (state)
            logger:default("Accessory Server State did update: " .. state .. ".")
            plugins.handleState(state)
        end,
        sessionAccept = function ()
            logger:default("Session is accepted")
        end,
        sessionInvalidate = function ()
            logger:default("Session is invalidated")
        end
    }))
    hap.start(true)
end)
