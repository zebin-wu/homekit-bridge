local protocol = require "miio.protocol"

local device = {}

---@alias DeviceState
---| '"NOINIT"'
---| '"IDLE"'
---| '"BUSY"'

---Create a device object.
---@param addr string Device address.
---@param token string Device token.
---@return Device obj Device object.
function device.create(addr, token)

    ---@class Device:table Device
    local dev = {
        devid = nil,
        addr = addr,
        token = token,
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
            self.protocol:request(function (result, self, respCb, ...)
                respCb(result, ...)
                self.state = "IDLE"
                dispatch(self)
            end, deque(que))
            self.state = "BUSY"
        end
    end

    protocol.scan(function (addr, devid, stamp, self)
        self.devid = devid
        self.protocol = protocol.create(addr, devid, self.token, stamp)
        self.state = "IDLE"
        dispatch(self)
    end, 5, addr, dev)

    ---Start a request and ``respCb`` will be called when a response is received.
    ---@param respCb fun(result: any, ...) Response callback.
    ---@param method string The request method.
    ---@param params? table Array of parameters.
    function dev:request(respCb, method, params, ...)
        enque(self.cmdQue, method, params, self, respCb, ...)
        dispatch(self)
    end

    return dev
end

return device
