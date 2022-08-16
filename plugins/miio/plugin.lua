local hap = require "hap"
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

---Miio plugin configuration.
---@class MiioPluginConf:PluginConf
---
---@field region '"cn"'|'"de"'|'"us"'|'"ru"'|'"tw"'|'"sg"'|'"in"'|'"i2"' Server region.
---@field username string User ID or email.
---@field password string User password.
---@field ssid string Wi-Fi SSID.

---Generate accessory via configuration.
---@param conf MiioAccessoryConf Accessory configuration.
---@return HAPAccessory accessory
local function gen(conf)
    local obj = device.create(conf.addr, conf.token)
    local info = obj:getInfo()
    local product = require("miio." .. info.model)
    conf.sn = info.mac:gsub(":", "")
    local handle = nvs.open(conf.sn)
    conf.aid = hapUtil.getBridgedAccessoryIID(handle)
    conf.iids = hapUtil.getInstanceIDs(handle)
    return product.gen(obj, info, conf)
end

---Initialize plugin.
---@param conf MiioPluginConf Plugin configuration.
---@return HAPAccessory[] bridgedAccessories Bridges Accessories.
function M.init(conf)
    logger:info("Initialing ...")

    local confs = {}
    do
        local devices
        do
            local session <close> = cloudapi.session(conf.region, conf.username, conf.password)
            devices = session:getDevices("wifi")
        end
        collectgarbage()
        local ssid = conf.ssid
        for _, device in ipairs(devices) do
            if device.ssid == ssid then
                tinsert(confs, {
                    addr = device.localip,
                    token = device.token,
                    name = device.name
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
