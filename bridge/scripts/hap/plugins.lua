local util = require "util"
local traceback = debug.traceback

local M = {}

local logger = log.getLogger("hap.plugins")

---@class PluginConf:table Plugin configuration.

---@class Plugin:table Plugin.
---
---@field init fun(conf: PluginConf) Initialize plugin and generate accessories in initialization.

local priv = {
    plugins = {},   ---@type table<string, Plugin>
}

---Load plugin.
---@param name string Plugin name.
local function loadPlugin(name, conf)
    local plugin = priv.plugins[name]
    if plugin then
        error("Plugin is already loaded.")
    end

    plugin = require(name .. ".plugin")
    if util.isEmptyTable(plugin) then
        error(("No fields in plugin '%s'."):format(name))
    end
    local fields = {
        init = "function"
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
    logger:info(("Plugin '%s' initializing ..."):format(name))
    local accessories = plugin.init(conf)
    logger:info(("Plugin '%s' initialized."):format(name))
    priv.plugins[name] = plugin
    return accessories
end

---Load plugins and generate bridged accessories.
---@param pluginConfs table<string, PluginConf> Plugin configurations.
---@return HAPAccessory[] bridgedAccessories # Bridges Accessories.
function M.init(pluginConfs)
    local accessories = {}
    if pluginConfs then
        for name, conf in pairs(pluginConfs) do
            local success, result = xpcall(loadPlugin, traceback, name, conf)
            if success == false then
                logger:error(result)
            end
            for _, accessory in ipairs(result) do
                table.insert(accessories, accessory)
            end
        end
    end
    local loaded = package.loaded
    for name, _ in pairs(loaded) do
        loaded[name] = nil
    end
    collectgarbage()
    return accessories
end

return M
