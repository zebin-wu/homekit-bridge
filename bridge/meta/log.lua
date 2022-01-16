---@meta

---@class loglib
log = {}

---Return a logger with the specified name, creating it if necessary.
---
---If no name is specified, return the default logger.
---@param name? string the specified name
---@return logger
---@nodiscard
function log.getLogger(name) end

---@class logger
local logger = {}

---Log with debug level.
---@param s string
function logger:debug(s) end

---Log with info level.
---@param s string
function logger:info(s) end

---Log with default level.
---@param s string
function logger:default(s) end

---Log with error level.
---@param s string
function logger:error(s) end

---Log with fault level.
---@param s string
function logger:fault(s) end

return log
