---@meta

---@class corelib
core = {}

---@class Timer:userdata Timer.
---
---A timer schedules an async Lua callback to be resumed by the bridge run loop.
---The callback itself still runs inside a bridge-managed coroutine created from C.
---Lua code can hold and restart the timer, but it does not own the coroutine
---used to execute the callback.
local timer = {}

---@class Future:userdata Future.
---
---A one-shot result container.
---
---Use a future when one code path starts an asynchronous operation and another
---code path will complete it exactly once later.
---
---State machine:
---1. A new future starts in the `pending` state.
---2. `resolve(...)` completes it successfully and caches the result values.
---3. `reject(...)` completes it unsuccessfully and caches the rejection values.
---4. Once completed, the future never changes state again.
---
---Waiting semantics:
---1. `wait()` suspends only while the future is still pending.
---2. After completion, `wait()` returns immediately with the cached result.
---3. Multiple coroutines may wait on the same future. They all observe the same
---   completion outcome.
---4. `wait()` returns `true, ...` for resolved futures and `false, ...` for
---   rejected futures.
local future = {}

---@class Event:userdata Event.
---
---A sticky level-triggered event.
---
---Use an event to represent a shared state transition such as "connected",
---"initialized", or "shutdown requested".
---
---State machine:
---1. A new event starts cleared.
---2. `set()` marks it as signaled.
---3. `clear()` marks it as unsignaled again.
---
---Waiting semantics:
---1. `wait()` returns immediately when the event is already set.
---2. Otherwise `wait()` suspends until some code path calls `set()`.
---3. `set()` wakes all current waiters.
---4. Future waiters also pass immediately until `clear()` is called.
local event = {}

---@class Queue:userdata Queue.
---
---A bounded FIFO message queue.
---
---Use a queue when values must be buffered and consumed in order.
---
---Semantics:
---1. Each `send(...)` appends one message to the tail of the queue.
---2. Each `recv()` removes one message from the head of the queue.
---3. A queued message may contain zero or more Lua values.
---4. `recv()` suspends when the queue is empty.
---5. `send(...)` errors when the queue is full.
---6. If receivers are already waiting and the queue is empty, a send may be
---   handed directly to the oldest waiting receiver instead of being buffered.
local queue = {}

---Get current time in milliseconds.
function core.time() end

---Cause normal program termination.
function core.exit() end

---Register a function to be called at normal program termination.
---@param func fun()
function core.atexit(func) end

---Whether the current Lua context is allowed to yield.
---
---This is useful when a helper can run in both synchronous and asynchronous
---contexts and must decide whether it can safely call APIs such as `sleep()`,
---`future:wait()`, `event:wait()`, or `queue:recv()`.
---
---@return boolean
---@nodiscard
function core.isYieldable() end

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

---Resolve the future successfully.
---
---This operation is one-shot. Calling it after the future has already completed
---raises an error.
---
---@param ... any
function future:resolve(...) end

---Reject the future.
---
---This operation is one-shot. Calling it after the future has already completed
---raises an error.
---
---@param ... any
function future:reject(...) end

---Wait for the future to complete.
---
---The current coroutine must be yieldable if the future is still pending.
---Resolved futures return `true, ...`; rejected futures return `false, ...`.
---
---@return boolean ok
---@return ...
---@nodiscard
function future:wait() end

---Get the current future state.
---
---@return '"pending"'|'"resolved"'|'"rejected"'
---@nodiscard
function future:state() end

---Create a future.
---
---@return Future
---@nodiscard
function core.createFuture() end

---Wait until the event becomes set.
---
---The current coroutine must be yieldable if the event is still clear.
---
---@nodiscard
function event:wait() end

---Set the event and wake all current waiters.
---
---After the event is set, later calls to `wait()` also return immediately until
---the event is cleared again.
function event:set() end

---Clear the event.
function event:clear() end

---Whether the event is currently set.
---
---@return boolean
---@nodiscard
function event:isSet() end

---Create an event.
---
---@return Event
---@nodiscard
function core.createEvent() end

---Send one message to the queue.
---
---The message may contain zero or more Lua values. If the queue is full and no
---receiver can take the message immediately, this method raises an error.
---
---@param ... any
function queue:send(...) end

---Receive one message from the queue.
---
---The current coroutine must be yieldable if the queue is empty.
---
---@return ...
---@nodiscard
function queue:recv() end

---The current number of buffered messages.
---
---@return integer
---@nodiscard
function queue:len() end

---The queue capacity in messages.
---
---@return integer
---@nodiscard
function queue:cap() end

---Create a bounded FIFO queue.
---
---@param size integer Queue capacity in messages.
---@return Queue
---@nodiscard
function core.createQueue(size) end

return core
