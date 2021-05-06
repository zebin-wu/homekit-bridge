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

---Check if table is empty.
---@param table table
---@return boolean
local function tableIsEmpty(table)
    if type(table) == "table" and _G.next(table) == nil then
        return true
    end
    return false
end

---Check if the plugin is valid.
---@param plugin Plugin
---@return boolean
local function pluginIsValid(plugin)
    if tableIsEmpty(plugin) then
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
    if tableIsEmpty(confs) then
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
