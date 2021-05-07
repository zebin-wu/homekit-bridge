local tab = require "tab"

local core = {}

local logger = log.getLogger("core")

---@class AccessoryConf:table Accessory configuration.
---
---@field plugin string Plugin name.

---@class Plugin:table Plugin.
---
---@field isInited fun():boolean Check plugin is inited.
---@field init fun():boolean Initialize plugin.
---@field deinit fun() Deinitialize plugin.
---@field gen fun(conf:AccessoryConf):Accessory|nil Generate accessory via configuration.

---Check if the plugin is valid.
---@param plugin Plugin
---@return boolean
local function pluginIsValid(plugin)
    if tab.isEmpty(plugin) then
        return false
    end
    return true
end

---Load plugin.
---@param name string plugin name.
---@return Plugin|nil
local function loadPlugin(name)
    local plugin = require(name .. ".plugin")
    if pluginIsValid(plugin) == false then
        logger:error(string.format("Plugin \"%s\" is invalid.", name))
        return nil
    end
    if plugin.isInited() == false then
        if plugin.init() == false then
            logger:error(string.format("Failed to init plugin \"%s\"", name))
            return nil
        end
    end
    return plugin
end

---Generate bridged accessories.
---@param confs AccessoryConf Accessory configuration.
---@return Accessory[]
function core.gen(confs)
    if tab.isEmpty(confs) then
        return {}
    end
    local accessories = {}
    for i, conf in ipairs(confs) do
        local plugin = loadPlugin(conf.plugin)
        if plugin ~= nil then
            table.insert(accessories, plugin.gen(conf))
        end
    end
    return accessories
end

return core
