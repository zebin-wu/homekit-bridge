local util = require "util"

local plugins = {}

local logger = log.getLogger("plugins")
local tinsert = table.insert

---@alias PluginState
---|'"INITED"'
---|'"PENDING"'
---|'"DONE"'

---@class AccessoryConf:table Accessory configuration.
---
---@field plugin string Plugin name.

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
    ---@type table<string, PluginState>
    states = {},
    ---@type table<string, Plugin>
    pendings = {},
    accessories = {},
    done = nil
}

---Load plugin.
---@param name string Plugin name.
---@param conf? PluginConf Plugin configuration.
---@return Plugin|nil
local function loadPlugin(name, conf)
    if priv.states[name] then
        return require(name .. ".plugin")
    end

    local plugin = require(name .. ".plugin")
    if util.isEmptyTable(plugin) then
        return nil
    end
    local fields = {
        init = "function",
        gen = "function",
        isPending = "function",
    }
    for k, t in pairs(fields) do
        if not plugin[k] then
            logger:error(("No field '%s' in plugin '%s'"):format(k, name))
            return nil
        end
        local _t = type(plugin[k])
        if _t ~= t then
            logger:error(("%s.%s: type error, expected %s, got %s"):format(name, k, t, _t))
            return nil
        end
    end
    if plugin.init(conf) == false then
        logger:error(("Failed to init plugin '%s'"):format(name))
        return nil
    end
    priv.states[name] = "INITED"
    return plugin
end

---Report generated bridged accessory.
---@param name string Plugin name.
---@param accessory HapAccessory Accessory.
function plugins.report(name, accessory)
    assert(priv.states[name] == "PENDING")
    tinsert(priv.accessories, accessory)

    if loadPlugin(name).isPending() == false then
        priv.states[name] = "DONE"
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
            if plugin ~= nil then
                tinsert(priv.accessories, plugin.gen(conf))
            end
        end
    end

    local states = priv.states

    for name, _ in pairs(states) do
        if loadPlugin(name).isPending() == true then
            states[name] = "PENDING"
            priv.pendings[name] = true
        else
            states[name] = "DONE"
        end
    end

    if util.isEmptyTable(priv.pendings) then
        done(priv.accessories)
        priv.accessories = {}
    else
        priv.done = done
    end
end

---Handle HAP server state.
---@param state HapServerState
function plugins.handleState(state)
    for name, st in pairs(priv.states) do
        assert(st == "DONE")
        loadPlugin(name).handleState(state)
    end
end

return plugins
