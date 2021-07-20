---@class timerlib
local timer = {}

---@class _timer:userdata Timer object.
local _timer = {}

---Create a timer.
---@param ms integer Monotonic trigger time in milliseconds.
---@param cb fun(...) Function to call when the timer expires.
---@vararg any Arguments passed to the callback.
---@return _timer|nil _timer
function timer.create(ms, cb, ...) end

---Cancel the timer before trigger.
function _timer:cancel() end

return timer
