local util = require "util"
local hap = require "hap"

local plugins = {}

local logger = log.getLogger("plugins")
local tinsert = table.insert

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
---@field init fun(conf: PluginConf|nil): boolean Initialize plugin.
---@field gen fun(conf: AccessoryConf): HapAccessory|nil Generate accessory via configuration.
---@field isPending fun(): boolean Whether the accessory is waiting to be generated.
---@field handleState fun(state: HapServerState) Handle HAP server state.

local priv = {
    plugins = {},   ---@type table<string, Plugin>
    pendings = {},  ---@type table<string, Plugin>
    accessories = {},
    done = nil
}

---Load plugin.
---@param name string Plugin name.
---@param conf? PluginConf Plugin configuration.
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
        isPending = "function",
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
    if plugin.init(conf) == false then
        error(("Failed to init plugin '%s'"):format(name))
    end
    priv.plugins[name] = plugin
    return plugin
end

---Report generated bridged accessory.
---@param name string Plugin name.
---@param accessory HapAccessory Accessory.
function plugins.report(name, accessory)
    tinsert(priv.accessories, accessory)

    local plugin = loadPlugin(name)
    if plugin.isPending() == false then
        priv.pendings[name] = nil
        if util.isEmptyTable(priv.pendings) then
            priv.done(priv.accessories)
            priv.accessories = {}
            priv.done = nil
        end
    end
end

---Load plugins and generate bridged accessories.
---@param pluginConfs PluginConf[] Plugin configurations.
---@param accessoryConfs AccessoryConf[] Accessory configurations.
---@param done async fun(bridgedAccessories: HapAccessory[]) Function called after the bridged accessories is generated.
function plugins.start(pluginConfs, accessoryConfs, done)
    if pluginConfs then
        for _, conf in ipairs(pluginConfs) do
            loadPlugin(conf.name, conf)
        end
    end

    if accessoryConfs then
        for _, conf in ipairs(accessoryConfs) do
            local plugin = loadPlugin(conf.plugin)
            conf.aid = hap.getNewBridgedAccessoryID()
            local acc = plugin.gen(conf)
            if acc then
                tinsert(priv.accessories, acc)
            end
        end
    end

    local pendings = priv.pendings
    for name, plugin in pairs(priv.plugins) do
        if plugin.isPending() == true then
            pendings[name] = true
        end
    end

    if util.isEmptyTable(pendings) then
        done(priv.accessories)
        priv.accessories = {}
    else
        priv.done = done
    end
end

---Handle HAP server state.
---@param state HapServerState
function plugins.handleState(state)
    for _, plugin in pairs(priv.plugins) do
        plugin.handleState(state)
    end
    if state == "Running" then
        local loaded = package.loaded
        for name, _ in pairs(loaded) do
            loaded[name] = nil
        end
    end
end

return plugins
