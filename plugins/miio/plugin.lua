local config = require "config"
local hapUtil = require "hap.util"
local nvs = require "nvs"
local device = require "miio.device"
local cloudapi = require "miio.cloudapi"
local traceback = debug.traceback
local tinsert = table.insert

local M = {}
local logger = log.getLogger("miio.plugin")

---Miio accessory configuration.
---@class MiioAccessoryConf
---
---@field aid integer Accessory Instance ID.
---@field iids table<string, integer> Instance IDs.
---@field addr string Device address.
---@field token string Device token.
---@field sn string Accessory serial number.
---@field name string Accessory name.
---@field model string Accessory model.
---@field fw_ver string Firmware version.
---@field hw_ver string Hardware version.

---Generate accessory via configuration.
---@param conf MiioAccessoryConf Accessory configuration.
---@return HAPAccessory accessory
local function gen(conf)
    return require("miio." .. conf.model).gen(device.create(conf.addr, conf.token), conf)
end

---Initialize plugin.
---@return HAPAccessory[] bridgedAccessories Bridges Accessories.
function M.init()
    logger:info("Initialing ...")

    local confs = {}
    do
        local devices
        do
            local region = assert(config.get("miio.region"))
            local username = assert(config.get("miio.username"))
            local password = assert(config.get("miio.password"))
            local session <close> = cloudapi.session(region, username, password)
            devices = session:getDevices("wifi")
        end
        collectgarbage()
        local ssid = assert(config.get("miio.ssid"))
        for _, device in ipairs(devices) do
            if device.ssid == ssid then
                local sn = device.mac:gsub(":", "")
                local handle = nvs.open(sn)
                tinsert(confs, {
                    aid = hapUtil.getBridgedAccessoryIID(handle),
                    iids = hapUtil.getInstanceIDs(handle),
                    addr = device.localip,
                    token = device.token,
                    name = device.name,
                    model = device.model,
                    sn = sn,
                    fw_ver = device.extra.fw_version,
                    hw_ver = device.extra.mcu_version or "0",
                })
            end
        end
    end
    collectgarbage()

    local accessories = {}

    for _, conf in ipairs(confs) do
        local success, result = xpcall(gen, traceback, conf)
        if success == false then
            logger:error(result)
        else
            table.insert(accessories, result)
        end
    end
    return accessories
end

return M
