local socket = require "socket"
local core = require "core"

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
    for _, domain in ipairs({"IPV4", "IPV6"}) do
        local sock <close> = socket.create(type, domain)
    end
end

---Test socket.create() with invalid type.
do
    local success = pcall(socket.create, nil, "IPV4")
    assert(success == false)
end

---Test socket.create() with invalid address family.
do
    local success = pcall(socket.create, "TCP", nil)
    assert(success == false)
end

---Test calling socket.destroy() twice.
do
    local sock = socket.create("TCP", "IPV4")
    sock:destroy()
    local success = pcall(sock.destroy, sock)
    assert(success == false)
end

---Test socket.destroy() with a <close> socket.
do
    local sock <close> = socket.create("TCP", "IPV4")
    sock:destroy()
end

---Test socket.bind() with valid parameters.
for _, addr in ipairs({"127.0.0.1", "0.0.0.0"}) do
    for _, port in ipairs({0, 8888, 65535}) do
        local sock <close> = socket.create("TCP", "IPV4")
        sock:bind(addr, port)
    end
end

---Test socket.bind() with invalid address.
do
    local sock <close> = socket.create("TCP", "IPV4")
    for _, addr in ipairs({"", "1"}) do
        local success, result = pcall(sock.bind, sock, addr, 0)
        assert(success == false)
    end
end

---Test socket.bind() with invalid port.
do
    local sock <close> = socket.create("TCP", "IPV4")
    for _, port in ipairs({-1, 65536}) do
        local success, error = pcall(sock.bind, sock, "0.0.0.0", port)
        assert(success == false)
    end
end

---Test UDP socket echo
do
    local server = socket.create("UDP", "IPV4")
    server:bind("127.0.0.1", 8888)
    core.createTimer(function ()
        while true do
            local msg, addr, port = server:recvfrom(1024)
            if #msg == 0 then
                server:destroy()
                return
            end
            assert(server:sendto(msg, addr, port) == #msg)
        end
    end):start(0)
    local client <close> = socket.create("UDP", "IPV4")
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
    local listener = socket.create("TCP", "IPV4")
    listener:bind("127.0.0.1", 8888)
    listener:listen(1024)
    core.createTimer(function ()
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
    local client <close> = socket.create("TCP", "IPV4")
    client:connect("127.0.0.1", 8888)
    for i = 1, 5, 1 do
        local msg = fillStr(1024)
        assert(client:send(msg) == #msg)
        assert(client:recv(1024) == msg)
    end
    assert(client:send("") == 0)
end
