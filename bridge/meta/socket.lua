---@meta

---@class socketlib
local M = {}

---@class Socket:userdata
local socket = {}

---Set the timeout.
---@param ms integer Maximum time blocked in milliseconds.
function socket:settimeout(ms) end

---Enable broadcast.
function socket:enablebroadcast() end

---Bind a socket to a local IP address and port.
---@param addr string Local address to use.
---@param port integer Local port number, in host order.
function socket:bind(addr, port) end

---Set the remote address and/or port.
---@param addr string Remote address to use.
---@param port integer Remote port number, in host order.
function socket:connect(addr, port) end

---Listen for connections.
---@param backlog integer The maximum length to which the queue of pending connections for socket may grow.
function socket:listen(backlog) end

---Accept a connection on a socket.
---@return Socket object Socket object.
---@return string addr Remote address.
---@return integer port Remote port.
---@nodiscard
function socket:accept() end

---Send data.
---@param data string The data to be sent.
---@return integer len Sent length.
function socket:send(data) end

---Send all the data.
---
---This function will return after all the data sent.
---@param data string The data to be sent.
function socket:sendall(data) end

---Send data to remote addr and port.
---@param data string The data to be sent.
---@param addr string Remote address to use.
---@param port integer Remote port number, in host order.
---@return integer len Sent length.
function socket:sendto(data, addr, port) end

---Receive data from a socket.
---@param maxlen integer The max length of the data.
---@return string data The received data.
---@nodiscard
function socket:recv(maxlen) end

---Receive data from a socket.
---@param maxlen integer The max length of the message.
---@return string data The received message.
---@return string addr The remote address.
---@return integer port The remote port.
---@nodiscard
function socket:recvfrom(maxlen) end

--Whether the socket is readable.
---@return boolean
function socket:readable() end

---Destroy the socket object.
function socket:destroy() end

---Create an endpoint for communication.
---@param type '"TCP"'|'"UDP"' Socket type.
---@param family '"IPV4"'|'"IPV6"' Address family.
---@return Socket object Socket object.
---@nodiscard
function M.create(type, family) end

return M
