---non-volatile storage.
---@class nvslib
local nvs = {}

---@class NVSHandle:userdata non-volatile storage handle.
local handle = {}

---Open a non-volatile storage handle with a given namespace.
---@param namespace string
---@return NVSHandle
function nvs.open(namespace) end

---Fetch the value of a key in the namespace.
---@param key string
---@return boolean|number|string value
function handle:get(key) end

---Set the value of a key in the namespace.
---@param key string
---@param value boolean|number|string
function handle:set(key, value) end

---Remove the value of a key in the namesapce.
---@param key string
function handle:remove(key) end

---Erase all key-value pairs in the namespace.
function handle:erase() end

---Write any pending changes to non-volatile storage.
function handle:commit() end

---Close the storage namespace and free any allocated resources.
function handle:close() end

return nvs
