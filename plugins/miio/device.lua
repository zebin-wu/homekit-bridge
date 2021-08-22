local protocol = require "miio.protocol"

local device = {}

---@alias DeviceState
---| '"NOINIT"'
---| '"IDLE"'
---| '"BUSY"'

---Create a device object.
---@param done fun(...) Callback will be called after the device is created.
---@param addr string Device address.
---@param token string Device token.
---@return DeviceObject obj Device object.
function device.create(done, addr, token, ...)
    assert(type(done) == "function")
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class DeviceObject:table Device object.
    local o = {
        devid = nil,
        state = "NOINIT",
        cmdQue = {first = 0, last = 0}
    }

    local function enque(que, ...)
        local last = que.last
        que[last] = {...}
        que.last = last + 1
    end

    local function deque(que)
        local first = que.first
        local cmd = que[first]
        que[first] = nil
        que.first = first + 1
        return table.unpack(cmd)
    end

    local function dispatch(self)
        if self.state ~= "IDLE" then
            return
        end
        local que = self.cmdQue
        if que.first ~= que.last then
            self.protocol:request(function (err, result, self, respCb, ...)
                respCb(err, result, ...)
                self.state = "IDLE"
                dispatch(self)
            end, deque(que))
            self.state = "BUSY"
        end
    end

    protocol.scan(function (addr, devid, stamp, self, token)
        self.protocol = protocol.create(addr, devid, token, stamp)
        self.state = "IDLE"
        dispatch(self)
    end, 5, addr, o, token)

    ---Start a request and ``respCb`` will be called when a response is received.
    ---@param respCb fun(err: MiioError|string|nil, result: any, ...) Response callback.
    ---@param method string The request method.
    ---@param params? table Array of parameters.
    function o:request(respCb, method, params, ...)
        assert(type(respCb) == "function")

        enque(self.cmdQue, method, params, self, respCb, ...)
        dispatch(self)
    end

    o:request(function (err, result, self, done, ...)
        self.info = result
        done(...)
    end, "miIO.info", nil, o, done, ...)

    return o
end

return device
