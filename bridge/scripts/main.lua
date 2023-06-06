local config = require "config"
local plugins = require "hap.plugins"
local hap = require "hap"
local chip = require "chip"
local netlink = require "netlink"

local logger = log.getLogger()

-- Wait for the network link is ready.
if not netlink.isUp() then netlink.waitUp() end

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
