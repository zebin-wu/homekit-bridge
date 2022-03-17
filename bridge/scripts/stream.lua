local dns = require "dns"
local ssl = require "ssl"
local socket = require "socket"

local stream = {}

---@class StreamClient:StreamClientPriv Stream client.
local client = {}

---Set the timeout.
---@param ms integer Maximum time blocked in milliseconds.
function client:settimeout(ms)
    self.sock:settimeout(ms)
end

---Send all the data.
---
---This function will return after all the data sent.
---@param data string The data to be sent.
function client:sendall(data)
    local sslctx = self.sslctx
    if sslctx then
        data = sslctx:encrypt(data)
    end
    self.sock:sendall(data)
end

local function recv(sock, sslctx)
    local data = sock:recv(1024)
    if #data ~= 0 and sslctx then
        data = sslctx:decrypt(data)
    end
    return data
end

---Receive all data.
---@return string data The received data.
---@nodiscard
function client:recvall()
    local sock = self.sock
    local sslctx = self.sslctx
    local output = recv(sock, sslctx)
    while sock:readable() do
        local data = recv(sock, sslctx)
        if data == "" then
            break
        end
        output = output .. data
    end
    return output
end

---Create a stream.
---@param type '"TCP"'|'"TLS"'|'"DTLS"'
---@param host string Server host name or IP address.
---@param port integer Remote port number, in host order.
---@param timeout integer Timeout period (in milliseconds).
---@return StreamClient
---@nodiscard
function stream.client(type, host, port, timeout)
    local addr, family = dns.resolve(host, timeout)
    local security = false
    local socktype
    if type == "TLS" then
        security = true
        socktype = "TCP"
    elseif type == "DTLS" then
        security = true
        socktype = "UDP"
    else
        socktype = type
    end
    local sock = socket.create(socktype, family)
    sock:settimeout(timeout)
    sock:connect(addr, port)

    ---@class StreamClientPriv
    local o = {
        sock = sock,
    }

    if security then
        local sslctx = ssl.create(type, "client", host ~= addr and host or nil)
        do
            local ds1 = sslctx:handshake()
            sock:sendall(ds1)
        end
        while not sslctx:finshed() do
            local ds2 = sock:recv(1024)
            local ds3 = sslctx:handshake(ds2)
            if ds3 then
                sock:sendall(ds3)
            end
        end

        o.sslctx = sslctx
    end

    setmetatable(o, {
        __index = client,
    })

    return o
end

return stream
