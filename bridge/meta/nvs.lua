---@meta

---@class nvslib:table non-volatile storage.
local nvs = {}

---@class NVSHandle:userdata non-volatile storage handle.
local handle = {}

---@alias NVSHandleMode
---|>'"rw"'     # Read or write.
---| '"r"'      # Read only.

---Open a non-volatile storage handle with a given namespace.
---@param namespace string
---@param mode? NVSHandleMode
---@return NVSHandle handle
---@nodiscard
function nvs.open(namespace, mode) end

---Fetch the value of a key in the namespace.
---@param key string
---@return boolean|number|table|string|nil value
---@nodiscard
function handle:get(key) end

---Set the value of a key in the namespace.
---@param key string
---@param value boolean|number|table|string|nil
function handle:set(key, value) end

---Erase all key-value pairs in the namespace.
function handle:erase() end

---Write any pending changes to non-volatile storage.
function handle:commit() end

---Close the storage namespace and free any allocated resources.
function handle:close() end

return nvs
