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

---Write data.
---@param data string The data to be write.
function client:write(data)
    local sslctx = self.sslctx
    self.sock:sendall(sslctx and sslctx:encrypt(data) or data)
end

---Read data.
---@param maxlen integer The max length of the data.
---@return string data The read data.
---@nodiscard
function client:read(maxlen)
    local sock = self.sock
    local sslctx = self.sslctx
    if not sslctx then
        return sock:recv(maxlen)
    end
    local readbuf = self.readbuf
    while #readbuf < maxlen do
        local data = sock:recv(maxlen)
        if #data == 0 then
            break
        end
        readbuf = readbuf .. sslctx:decrypt(data)
        if not sock:readable() then
            break
        end
    end

    if #readbuf <= maxlen then
        self.readbuf = ""
        return readbuf
    else
        local out = readbuf:sub(1, maxlen)
        self.readbuf = readbuf:sub(maxlen + 1)
        return out
    end
end

---Read a line.
---@param sep? string Split string, default is "\n".
---@param skip? boolean Whether to skip the split string, default is false.
---@return string line
function client:readline(sep, skip)
    sep = sep or "\n"
    local readbuf = self.readbuf
    local data
    if #readbuf > 0 then
        self.readbuf = ""
        data = readbuf
    else
        data = self:read(1024)
        if #data == 0 then
            error("read EOF")
        end
    end
    local seplen = #sep
    local init = 1
    while true do
        local s, e = data:find(sep, init, true)
        if s then
            local line = data:sub(1, skip and s - 1 or e)
            if e < #data then
                readbuf = self.readbuf
                self.readbuf = readbuf .. data:sub(e + 1)
            end
            return line
        else
            if seplen < e then
                init = e + 2 - seplen
            end
            local _data = self:read(1024)
            if #_data == 0 then
                error("read EOF")
            end
            data = data .. _data
        end
    end
end

---Close the connection.
function client:close()
    self.sock:destroy()
end

---Create a stream client and connect to the host.
---@param type '"TCP"'|'"TLS"'|'"DTLS"'
---@param host string Server host name or IP address.
---@param port integer Remote port number, in host order.
---@param timeout integer Timeout period (in milliseconds).
---@return StreamClient
---@nodiscard
function stream.client(type, host, port, timeout)
    local starttime = core.time()
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
    sock:settimeout(timeout - core.time() + starttime)
    sock:connect(addr, port)
    sock:settimeout(timeout)

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
        o.readbuf = ""
    end

    return setmetatable(o, {
        __index = client,
        __close = client.close
    })
end

return stream
