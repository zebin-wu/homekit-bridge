---@class timerlib
local timer = {}

---@class TimerHandle:userdata Timer handle.
local handle = {}

---Create a timer.
---@param ms integer Monotonic trigger time in milliseconds.
---@param cb fun(...) Function to call when the timer expires.
---@vararg any Arguments passed to the callback.
---@return TimerHandle handle
function timer.create(ms, cb, ...) end

---Destroy a timer before trigger.
function handle:destroy() end

return timer
