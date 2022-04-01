local stream = require "stream"
local urllib = require "url"
local tonumber = tonumber
local tinsert = table.insert
local ipairs = ipairs
local pairs = pairs
local assert = assert
local type = type
local error = error

---@class httpclib HTTP client library.
local M = {}

---@alias HTTPMethod
---| '"GET"'
---| '"POST"'
---| '"HEAD"'
---| '"OPTIONS"'
---| '"PUT"'
---| '"PATCH"'
---| '"DELETE"'

---@class HTTPClient:HTTPClientPriv HTTP client.
local client = {}

---Set the timeout.
---@param ms integer Maximum time blocked in milliseconds.
function client:settimeout(ms)
    self.sc:settimeout(ms)
end

---Start a HTTP request.
---@param method HTTPMethod The request method.
---@param path string The request path.
---@param headers? table<string, string> The request headers.
---@param body? string|fun():string The request body.
---@return integer code The response status code.
---@return table<string, string> headers The response headers.
---@return string|fun():string|nil body The response body.
---@nodiscard
function client:request(method, path, headers, body)
    local sc = self.sc

    local chunked = false
    do
        headers = headers or {}
        if not headers["Host"] then
            headers["Host"] = self.host
        end
        if body then
            if type(body) == "function" then
                chunked = true
                headers["Transfer-Encoding"] = "chunked"
            else
                headers["Content-Length"] = #body
            end
        else
            headers["Content-Length"] = 0
        end
        sc:write(("%s %s HTTP/1.1\r\n"):format(method, path))
        for k, v in pairs(headers) do
            if type(v) == "table" then
                for _, v in ipairs(v) do
                    sc:write(("%s:%s\r\n"):format(k, v))
                end
            else
                sc:write(("%s:%s\r\n"):format(k, v))
            end
        end
        sc:write("\r\n")
    end

    if body then
        if chunked then
            while true do
                local chunk = body()
                if #chunk > 0 then
                    sc:write(("%X\r\n%s\r\n"):format(#chunk, chunk))
                else
                    sc:write("\r\n")
                    break
                end
            end
        else
            sc:write(body)
        end
    end

    local line = sc:readline("\r\n", true)
    local code, _ = line:match("HTTP/[%d%.]+%s+([%d]+)%s+(.*)$")
    code = assert(tonumber(code))

    headers = {}
    while true do
        line = sc:readline("\r\n", true)
        if #line == 0 then
            break
        end
        local k, v = line:match("^(.-):%s*(.*)")
        local t = type(headers[k])
        if t == "table" then
            tinsert(headers[k], v)
        elseif t == "string" then
            headers[k] = { headers[k], v }
        else
            headers[k] = v
        end
    end

    local length = headers["Content-Length"]
    if length then
        length = tonumber(length)
    end

    local mode = headers["Transfer-Encoding"]
    if mode then
        if mode ~= "identity" and mode ~= "chunked" then
            error("unsupport Transfer-Encoding: " .. mode)
        end
    end
    if mode == "chunked" then
        body = function ()
            local size = tonumber(sc:readline("\r\n", true), 16)
            local chunk = size > 0 and sc:read(size, true) or ""
            sc:readline("\r\n", true)
            return chunk
        end
    elseif length then
        body = sc:read(length, true)
    elseif code == 204 or code == 304 or code < 200 then
        body = nil
    else
        body = sc:readall()
    end

    return code, headers, body
end

---Close the connection.
function client:close()
    self.sc:close()
end

---Connect to HTTP server and return a client.
---@param host string Server host name or IP address.
---@param port integer Remote port number, in host order.
---@param tls boolean Whether to enable SSL/TLS.
---@param timeout integer Timeout period (in milliseconds).
---@return HTTPClient client HTTP client.
---@nodiscard
function M.connect(host, port, tls, timeout)
    ---@class HTTPClientPriv:table
    local o = {
        host = host,
        timeout = timeout
    }
    local sc = stream.client(tls and "TLS" or "TCP", host, port, timeout)
    o.sc = sc

    return setmetatable(o, {
        __index = client,
        __close = client.close
    })
end

---Parse a URL.
---@param url string
---@return string host
---@return integer port
---@return string path
local function parseURL(url)
    local u = urllib.parse(url)
    local host = u.host
    assert(type(host) == "string", "missing host in url")

    local port = u.port
    local scheme = u.scheme
    if not port and scheme then
        if scheme == "http" then
            port = 80
        elseif scheme == "https" then
            port = 443
        else
            error("invalid scheme: " .. scheme)
        end
    end
    port = port or 80

    local path = u.path or "/"
    if u.query then
        local query = tostring(u.query)
        if query ~= "" then
            path = path .. "?" .. query
        end
    end

    return host, port, path
end

---Get chunk.
---@param body fun():string
---@return string
local function getChunk(body)
    local chunk = ""
    while true do
        local bytes = body()
        if bytes == "" then
            break
        end
        chunk = chunk .. bytes
    end
    return chunk
end

---Start a HTTP request and wait for the response back.
---@param method HTTPMethod The request method.
---@param url string URL string.
---@param timeout? integer Timeout period (in milliseconds).
---@param headers? table<string, string> The request headers.
---@param body? string|fun():string The request body.
---@return integer code The response status code.
---@return table<string, string> headers The response headers.
---@return string|nil body The response body.
---@nodiscard
function M.request(method, url, timeout, headers, body)
    local host, port, path = parseURL(url)
    timeout = timeout or 5000
    local hc <close> = M.connect(host, port, port == 443, timeout)
    hc:settimeout(timeout)
    local code
    code, headers, body = hc:request(method, path, headers, body)
    if type(body) == "function" then
        body = getChunk(body)
    end
    return code, headers, body
end

---@class HTTPClientSession
local session = {}

---Start a HTTP request and wait for the response back.
---@param method HTTPMethod The request method.
---@param url string URL string.
---@param timeout? integer Timeout period (in milliseconds).
---@param headers? table<string, string> The request headers.
---@param body? string|fun():string The request body.
---@return integer code The response status code.
---@return table<string, string> headers The response headers.
---@return string|nil body The response body.
---@nodiscard
function session:request(method, url, timeout, headers, body)
    local host, port, path = parseURL(url)
    timeout = timeout or 5000

    local hc = self.hc
    if hc == nil or host ~= self.host or port ~= self.port then
        if hc then
            hc:close()
        end
        hc = M.connect(host, port, port == 443, timeout)
        hc:settimeout(timeout)
        self.hc = hc
        self.host = host
        self.port = port
        self.timeout = timeout
    elseif self.timeout ~= timeout then
        hc:settimeout(timeout)
        self.timeout = timeout
    end
    local code
    code, headers, body = hc:request(method, path, headers, body)
    if type(body) == "function" then
        body = getChunk(body)
    end
    if headers.connection == "close" then
        hc:close()
        self.hc = nil
        self.host = nil
        self.port = nil
    end
    return code, headers, body
end

function session:close()
    if self.hc then
        self.hc:close()
    end
end

---Create a HTTP Client session.
---@return HTTPClientSession
function M.session()
    return setmetatable({}, {
        __index = session,
        __close = session.close
    })
end

return M
