---@class httplib
local http = {}

---@alias HTTPMethod
---| '"GET"'
---| '"POST"'
---| '"HEAD"'
---| '"OPTIONS"'
---| '"PUT"'
---| '"PATCH"'
---| '"DELETE"'

---@class HTTPClient HTTP client.
local client = {}

---Connect to HTTP server and return a client.
---@param host string Server host name or IP address.
---@param tls boolean Whether to enable TLS.
---@param timeout? integer Timeout period (in milliseconds).
---@return HTTPClient client HTTP client.
---@nodiscard
function http.connect(host, tls, timeout) end

---Start a HTTP request.
---@param method HTTPMethod The request method.
---@param path string The request path.
---@param headers table<string, string> The request headers.
---@param content? string The request content.
---@return integer statuscode The response status code.
---@return table<string, string> headers The response headers.
---@return string content The response content.
---@nodiscard
function client:request(method, path, headers, content) end

function client:close() end

return http
