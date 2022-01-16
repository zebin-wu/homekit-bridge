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

---Start a HTTP request.
---@param method HTTPMethod The request method.
---@param url string The request URL.
---@param headers table<string, string> The request headers.
---@param content? string The request content.
---@param timeout? integer Timeout period (in milliseconds).
---@return integer statuscode The response status code.
---@return table<string, string> headers The response headers.
---@return string content The response content.
---@nodiscard
function http.request(method, url, headers, content, timeout) end

return http
