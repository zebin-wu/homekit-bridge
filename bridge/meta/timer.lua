---@class timerlib
local timer = {}

---@class _timer:userdata Timer object.
local _timer = {}

---Create a timer.
---@param cb fun(arg?: any) Function to call when the timer expires.
---@param arg? any Argument passed to the callback.
---@return _timer _timer
function timer.create(cb, arg) end

---Start the timer.
---@param ms integer Monotonic trigger time in milliseconds.
function _timer:start(ms) end

---Cancel the timer before trigger.
function _timer:cancel() end

return timer
