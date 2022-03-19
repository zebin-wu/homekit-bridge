local stream = require "stream"

local httpc = {}

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

---Start a HTTP request.
---@param method HTTPMethod The request method.
---@param path string The request path.
---@param headers table<string, string> The request headers.
---@param content? string|fun():string The request content.
---@return integer code The response status code.
---@return table<string, string> headers The response headers.
---@return string|fun():string|nil content The response content.
---@nodiscard
function client:request(method, path, headers, content)
    local sc = self.sc

    do
        local chunked = false
        do
            headers = headers or {}
            if not headers.host then
                headers.host = self.host
            end
            if content then
                if type(content) == "function" then
                    chunked = true
                    headers["transfer-encoding"] = "chunked"
                else
                    headers["content-length"] = #content
                end
            else
                headers["content-length"] = 0
            end
            sc:write(("%s %s HTTP 1.1\r\n"):format(method, path))
            for k, v in pairs(headers) do
                sc:write(("%s:%s\r\n"):format(k, v))
            end
            sc:write("\r\n\r\n")
        end

        if content then
            if chunked then
                while true do
                    local chunk = content()
                    if #chunk > 0 then
                        sc:write(("%X\r\n%s\r\n"):format(#chunk, chunk))
                    else
                        sc:write("\r\n")
                        break
                    end
                end
            else
                sc:write(content)
            end
        end
    end

    do
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
            headers[k:lower()] = v
        end

        local length = headers["content-length"]
        if length then
            length = tonumber(length)
        end

        local mode = headers["transfer-encoding"]
        if mode then
            if mode ~= "identity" and mode ~= "chunked" then
                error("unsupport transfer-encoding")
            end
        end
        if mode == "chunked" then
            return code, headers, function ()
                local size = tonumber(sc:readline("\r\n", true), 16)
                return sc:read(size)
            end
        end

        if length then
            return code, headers, sc:read(length)
        else
            return code, headers
        end
    end
end

function client:close()
    self.sc:close()
end

---Connect to HTTP server and return a client.
---@param host string Server host name or IP address.
---@param tls boolean Whether to enable SSL/TLS.
---@param timeout integer Timeout period (in milliseconds).
---@return HTTPClient client HTTP client.
---@nodiscard
function httpc.connect(host, tls, timeout)
    ---@class HTTPClientPriv:table
    local o = {
        host = host,
        timeout = timeout
    }
    o.sc = stream.client(tls and "TLS" or "TCP", host, tls and 443 or 80, timeout)

    return setmetatable(o, {
        __index = client,
        __close = client.close
    })
end

return httpc
