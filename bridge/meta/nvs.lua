---@meta

---@class nvslib:table non-volatile storage.
local M = {}

---@class NVSHandle:userdata non-volatile storage handle.
local handle = {}

---Fetch the value of a key.
---@param key string
---@return any value
---@nodiscard
function handle:get(key) end

---Set the value of a key.
---@param key string
---@param value any
function handle:set(key, value) end

---Erase all key-value pairs.
function handle:erase() end

---Write any pending changes to non-volatile storage.
function handle:commit() end

---Close the handle and free any allocated resources.
function handle:close() end

---Open a non-volatile storage handle with a given namespace.
---@param namespace string
---@return NVSHandle handle
---@nodiscard
function M.open(namespace) end

return M
