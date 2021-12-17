---@class timelib
local time = {}

---@class Timer:userdata Timer.
local timer = {}

---Sleep for a specified number of milliseconds.
---@param ms integer Milliseconds.
function time.sleep(ms) end

---Create a timer.
---@param cb async fun(...) Function to call when the timer expires.
---@vararg any Arguments passed to the callback.
---@return Timer timer
function time.createTimer(cb, ...) end

---Start the timer.
---If the timer has already started, it will start again.
---@param ms integer The timeout period in milliseconds.
function timer:start(ms) end

---Stop the timer before trigger.
---If the timer has not started, nothing will happen.
function timer:stop() end

return time
