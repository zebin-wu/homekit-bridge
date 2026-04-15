local core = require "core"
local socket = require "socket"
local stream = require "stream"

local TIMEOUT = 1000

local function with_tcp_server(handler, test)
    local listener <close> = socket.create("TCP", "IPV4")
    listener:bind("127.0.0.1", 0)
    local _, port = listener:getsockname()
    listener:listen(1)

    core.createTimer(function ()
        local server <close> = listener:accept()
        server:settimeout(TIMEOUT)
        handler(server)
    end):start(0)

    test(port)
end

-- Tests stream.client() with invalid type.
do
    local success = pcall(stream.client, nil, "127.0.0.1", 1, TIMEOUT)
    assert(success == false)
end

-- Tests read() returning partial data when not requesting all bytes.
with_tcp_server(function (server)
    server:sendall("he")
    core.sleep(50)
    server:sendall("llo")
end, function (port)
    local client <close> = stream.client("TCP", "127.0.0.1", port, TIMEOUT)
    client:settimeout(TIMEOUT)
    assert(client:read(5) == "he")
    assert(client:read(3, true) == "llo")
end)

-- Tests readline() leaving unread bytes in the internal stash buffer.
with_tcp_server(function (server)
    server:sendall("header\r\nbody")
end, function (port)
    local client <close> = stream.client("TCP", "127.0.0.1", port, TIMEOUT)
    client:settimeout(TIMEOUT)
    assert(client:readline("\r\n", true) == "header")
    assert(client:read(4, true) == "body")
end)

-- Tests readline() when the separator is split across multiple reads.
with_tcp_server(function (server)
    server:sendall("abc\r")
    core.sleep(50)
    server:sendall("\nxyz")
end, function (port)
    local client <close> = stream.client("TCP", "127.0.0.1", port, TIMEOUT)
    client:settimeout(TIMEOUT)
    assert(client:readline("\r\n", true) == "abc")
    assert(client:read(3, true) == "xyz")
end)

-- Tests readall() collecting the remaining stream until EOF.
with_tcp_server(function (server)
    server:sendall("chunk1")
    core.sleep(20)
    server:sendall("chunk2")
end, function (port)
    local client <close> = stream.client("TCP", "127.0.0.1", port, TIMEOUT)
    client:settimeout(TIMEOUT)
    assert(client:readall() == "chunk1chunk2")
end)

-- Tests readline() raising an error on EOF without a separator.
with_tcp_server(function (server)
    server:sendall("tail")
end, function (port)
    local client <close> = stream.client("TCP", "127.0.0.1", port, TIMEOUT)
    client:settimeout(TIMEOUT)
    local success, err = pcall(client.readline, client, "\n", true)
    assert(success == false)
    assert(type(err) == "string")
    assert(err:find("read EOF", 1, true) ~= nil)
end)
