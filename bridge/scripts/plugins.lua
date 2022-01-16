local util = require "util"
local hap = require "hap"

local plugins = {}

local logger = log.getLogger("plugins")

---@class AccessoryConf:table Accessory configuration.
---
---@field plugin string Plugin name.
---@field aid integer Accessory Instance ID.
---@field name string Accessory name.

---@class PluginConf:table Plugin configuration.
---
---@field name string Plugin name.

---@class Plugin:table Plugin.
---
---@field init fun(conf: PluginConf|nil) Initialize plugin and generate accessories in initialization.
---@field gen fun(conf: AccessoryConf): HapAccessory Generate accessory via configuration.
---@field handleState fun(state: HapServerState) Handle HAP server state.

local priv = {
    plugins = {},   ---@type table<string, Plugin>
}

---Load plugin.
---@param name string Plugin name.
---@return Plugin|nil
local function loadPlugin(name, conf)
    local plugin = priv.plugins[name]
    if plugin then
        return plugin
    end

    plugin = require(name .. ".plugin")
    if util.isEmptyTable(plugin) then
        error(("No fields in plugin '%s'."):format(name))
    end
    local fields = {
        init = "function",
        gen = "function",
        handleState = "function"
    }
    for k, t in pairs(fields) do
        if not plugin[k] then
            error(("No field '%s' in plugin '%s'."):format(k, name))
        end
        local _t = type(plugin[k])
        if _t ~= t then
            error(("%s.%s: type error, expected %s, got %s."):format(name, k, t, _t))
        end
    end
    plugin.init(conf)
    priv.plugins[name] = plugin
    return plugin
end

---Load plugins and generate bridged accessories.
---@param pluginConfs PluginConf[] Plugin configurations.
---@param accessoryConfs AccessoryConf[] Accessory configurations.
---@return HapAccessory[] accessories
function plugins.start(pluginConfs, accessoryConfs)
    if pluginConfs then
        for _, conf in ipairs(pluginConfs) do
            loadPlugin(conf.name, conf)
        end
    end

    local accessories = {}

    if accessoryConfs then
        local tinsert = table.insert
        for _, conf in ipairs(accessoryConfs) do
            local plugin = loadPlugin(conf.plugin)
            conf.aid = hap.getNewBridgedAccessoryID()
            local success, result = xpcall(plugin.gen, debug.traceback, conf)
            if success == false then
                logger:error(result)
            else
                tinsert(accessories, result)
            end
        end
    end
    return accessories
end

---Handle HAP server state.
---@param state HapServerState
function plugins.handleState(state)
    for _, plugin in pairs(priv.plugins) do
        plugin.handleState(state)
    end
end

return plugins
