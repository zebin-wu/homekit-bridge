---@meta

---@class corelib
core = {}

---@class Timer:userdata Timer.
local timer = {}

---Get current time stamp.
function core.time() end

---Exit the host program.
function core.exit() end

---Sleep for a specified number of milliseconds.
---@param ms integer Milliseconds.
function core.sleep(ms) end

---Create a timer.
---@param cb async fun(...) Function to call when the timer expires.
---@param ... any Arguments passed to the callback.
---@return Timer timer
---@nodiscard
function core.createTimer(cb, ...) end

---Start the timer.
---If the timer has already started, it will start again.
---@param ms integer The timeout period in milliseconds.
function timer:start(ms) end

---Stop the timer before trigger.
---If the timer has not started, nothing will happen.
function timer:stop() end

return core
