local config = require "config"
local plugins = require "hap.plugins"
local hap = require "hap"
local chip = require "chip"

local logger = log.getLogger()

hap.start(
    hap.newAccessory(
        1,
        "Bridges",
        config.bridge.name or "HomeKit Bridge",
        chip.getInfo("mfg"),
        chip.getInfo("model"),
        chip.getInfo("sn"),
        ---@diagnostic disable-next-line: undefined-global
        _BRIDGE_VERSION,
        chip.getInfo("hwver"),
        {
            hap.AccessoryInformationService,
            hap.HAPProtocolInformationService,
            hap.PairingService,
        },
        function (request)
            logger:info("Identify callback is called.")
        end
    ),
    plugins.init(config.plugins),
    true,
    function (session)
        logger:default(("Session %p is accepted."):format(session))
    end,
    function (session)
        logger:default(("Session %p is invalidated."):format(session))
    end
)
