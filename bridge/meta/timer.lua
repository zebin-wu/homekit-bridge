---@class timerlib
local timer = {}

---@class TimerObj:userdata Timer object.
local obj = {}

---Create a timer object.
---@param cb fun(...) Function to call when the timer expires.
---@vararg any Arguments passed to the callback.
---@return TimerObj obj
function timer.create(cb, ...) end

---Start the timer.
---@param ms integer Monotonic trigger time in milliseconds.
function obj:start(ms) end

---Cancel the timer before trigger.
function obj:stop() end

return timer
