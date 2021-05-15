local util = require "util"

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

---Load plugin.
---@param name string plugin name.
---@return Plugin|nil
local function loadPlugin(name)
    local plugin = require(name .. ".plugin")
    if util.isEmptyTable(plugin) then
        return nil
    end
    local fields = {
        isInited = "function",
        init = "function",
        deinit = "function",
        gen = "function",
    }
    for k, t in pairs(fields) do
        if not plugin[k] then
            logger:error(string.format("No field '%s' in plugin '%s'", k, name))
            return nil
        end
        local _t = type(plugin[k])
        if _t ~= t then
            logger:error(string.format("%s.%s: type error, expected %s, got %s", name, k, t, _t))
            return nil
        end
    end
    if plugin.isInited() == false then
        if plugin.init() == false then
            logger:error(string.format("Failed to init plugin '%s'", name))
            return nil
        end
    end
    return plugin
end

---Generate bridged accessories.
---@param confs AccessoryConf Accessory configuration.
---@return Accessory[]
function core.gen(confs)
    if util.isEmptyTable(confs) then
        return {}
    end
    local accessories = {}
    for i, conf in ipairs(confs) do
        local plugin = loadPlugin(conf.plugin)
        if plugin ~= nil then
            table.insert(accessories, plugin.gen(conf))
        end
        collectgarbage()
    end
    return accessories
end

return core
