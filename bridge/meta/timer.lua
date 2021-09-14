---@class timerlib
local timer = {}

---@class TimerObj:userdata Timer object.
local obj = {}

---Create a timer object.
---@param cb fun(arg?: any) Function to call when the timer expires.
---@return TimerObj obj
function timer.create(cb) end

---Start the timer.
---@param ms integer Monotonic trigger time in milliseconds.
---@param arg? any Argument passed to the callback.
function obj:start(ms, arg) end

---Cancel the timer before trigger.
function obj:cancel() end

return timer
