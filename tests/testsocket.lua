local socket = require "socket"
local time = require "time"
local logger = log.getLogger("testsocket")

local function fillStr(n, fill)
    fill = fill or "0123456789"
    local s = ""
    while #s < n - #fill do
        s = s .. fill
    end
    return s .. fill:sub(0, n - #s)
end

---Test socket.create() with valid parameters.
for _, type in ipairs({"TCP", "UDP"}) do
    for _, domain in ipairs({"INET", "INET6"}) do
        local sock <close> = socket.create(type, domain)
    end
end

---Test socket.create() with invalid type.
do
    local success = pcall(socket.create, nil, "INET")
    assert(success == false)
end

---Test socket.create() with invalid domain.
do
    local success = pcall(socket.create, "TCP", nil)
    assert(success == false)
end

---Test calling socket.destroy() twice.
do
    local sock = socket.create("TCP", "INET")
    sock:destroy()
    local success = pcall(sock.destroy, sock)
    assert(success == false)
end

---Test socket.destroy() with a <close> socket.
do
    local sock <close> = socket.create("TCP", "INET")
    sock:destroy()
end

---Test socket.bind() with valid parameters.
for _, addr in ipairs({"127.0.0.1", "0.0.0.0"}) do
    for _, port in ipairs({0, 8888, 65535}) do
        local sock <close> = socket.create("TCP", "INET")
        sock:bind(addr, port)
    end
end

---Test socket.bind() with invalid address.
do
    local sock <close> = socket.create("TCP", "INET")
    for _, addr in ipairs({"", "1"}) do
        local success, result = pcall(sock.bind, sock, addr, 0)
        assert(success == false)
    end
end

---Test socket.bind() with invalid port.
do
    local sock <close> = socket.create("TCP", "INET")
    for _, port in ipairs({-1, 65536}) do
        local success, error = pcall(sock.bind, sock, "0.0.0.0", port)
        assert(success == false)
    end
end

---Test UDP socket echo
do
    local server = socket.create("UDP", "INET")
    server:bind("127.0.0.1", 8888)
    time.createTimer(function ()
        while true do
            local msg, addr, port = server:recvfrom(1024)
            if #msg == 0 then
                server:destroy()
                return
            end
            assert(server:sendto(msg, addr, port) == #msg)
        end
    end):start(0)
    local client <close> = socket.create("UDP", "INET")
    client:connect("127.0.0.1", 8888)
    for i = 1, 100, 1 do
        local msg = fillStr(1024)
        assert(client:send(msg) == #msg)
        assert(client:recv(1024) == msg)
    end
    assert(client:send("") == 0)
end

---Test TCP socket echo
do
    local listener = socket.create("TCP", "INET")
    listener:bind("127.0.0.1", 8888)
    listener:listen(1024)
    time.createTimer(function ()
        while true do
            local server = listener:accept()
            while true do
                local msg = server:recv(1024)
                if #msg == 0 then
                    server:destroy()
                    return
                end
                assert(server:send(msg) == #msg)
            end
        end
    end):start(0)
    local client <close> = socket.create("TCP", "INET")
    client:connect("127.0.0.1", 8888)
    for i = 1, 5, 1 do
        local msg = fillStr(1024)
        assert(client:send(msg) == #msg)
        assert(client:recv(1024) == msg)
    end
    assert(client:send("") == 0)
end
