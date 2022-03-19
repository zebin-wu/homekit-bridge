---@meta

---Message Queue.
---
---This message queue can be used for inter-coroutine communication.
---@class mqlib
local M = {}

---@class MessageQueue:userdata Message queue.
local mq = {}

---Send message.
---@param ... any
function mq:send(...) end

---Receive message.
---
---When the message queue is empty, the current coroutine
---waits here until a message is received.
---@return ...
function mq:recv() end

---Create a message queue.
---@param size integer Queue size.
---@return MessageQueue
function M.create(size) end

return M
