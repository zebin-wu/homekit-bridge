---Message Queue.
---
---This message queue can be used for inter-coroutine communication.
---@class mqlib
local mq = {}

---@class MessageQueue:userdata Message queue.
local _mq = {}

---Create a message queue.
---@param size integer Queue size.
---@return MessageQueue
function mq.create(size) end

---Send message.
---@param ... any
function _mq:send(...) end

---Receive message.
---
---When the message queue is empty, the current coroutine
---waits here until a message is received.
---@return ...
function _mq:recv() end

return mq
