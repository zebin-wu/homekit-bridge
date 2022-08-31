---@meta

---@class clilib command-line inteface.
local M = {}

---Register a command.
---@param cmd string
---@param help string
---@param hint string
---@param func fun(...: string)
function M.register(cmd, help, hint, func) end

return M
