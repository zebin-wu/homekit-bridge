---@meta

---@class corelib
core = {}

---@class Timer:userdata Timer.
local timer = {}

---@class MessageQueue:userdata Message queue.
local mq = {}

---Get current time in milliseconds.
function core.time() end

---Cause normal program termination.
function core.exit() end

---Register a function to be called at normal program termination.
---@param func fun()
function core.atexit(func) end

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

---Send message.
---@param ... any
function mq:send(...) end

---Receive message.
---
---When the message queue is empty, the current coroutine
---waits here until a message is received.
---@return ...
---@nodiscard
function mq:recv() end

---Create a message queue.
---@param size integer Queue size.
---@return MessageQueue
---@nodiscard
function core.createMQ(size) end

return core
