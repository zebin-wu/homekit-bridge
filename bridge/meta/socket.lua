---@class socketlib
local socket = {}

---@class Socket:userdata
local _socket = {}

---@alias SocketType
---|'"TCP"'
---|'"UDP"'

---@alias SocketDomain
---|'"INET"'    # IPv4 Internet protocols
---|'"INET6"'   # IPv6 Internet protocols

---Create an endpoint for communication.
---@param type SocketType
---@param domain SocketDomain
---@return Socket object Socket object.
function socket.create(type, domain) end

---Set the timeout.
---@param ms integer Maximum time blocked in milliseconds.
function _socket:settimeout(ms) end

---Enable broadcast.
function _socket:enablebroadcast() end

---Bind a socket to a local IP address and port.
---@param addr string Local address to use.
---@param port integer Local port number, in host order.
function _socket:bind(addr, port) end

---Set the remote address and/or port.
---@param addr string Remote address to use.
---@param port integer Remote port number, in host order.
function _socket:connect(addr, port) end

---Listen for connections.
---@param backlog integer The maximum length to which the queue of pending connections for socket may grow.
function _socket:listen(backlog) end

---Accept a connection on a socket.
---@return Socket object Socket object.
---@return string addr Remote address.
---@return integer port Remote port.
function _socket:accept() end

---Send a message.
---@param message string The message to be sent.
function _socket:send(message) end

---Send a message to remote addr and port.
---@param message string The message to be sent.
---@param addr string Remote address to use.
---@param port integer Remote port number, in host order.
function _socket:sendto(message, addr, port) end

---Receive a message from a socket.
---@param maxlen integer The max length of the message.
---@return string message The received message.
function _socket:recv(maxlen) end

---Receive a message from a socket.
---@param maxlen integer The max length of the message.
---@return string message The received message.
---@return string addr The remote address.
---@return integer port The remote port.
function _socket:recvfrom(maxlen) end

---Destroy the socket object.
function _socket:destroy() end

return socket
