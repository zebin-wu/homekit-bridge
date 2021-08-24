local protocol = require "miio.protocol"
local timer = require "timer"

local device = {}
local logger = log.getLogger("miio.device")

---@alias DeviceState
---| '"NOINIT"'
---| '"IDLE"'
---| '"BUSY"'

---@class MiioDeviceNetIf:table Device network interface.
---
---@field gw string gateway IP.
---@field localIp string local IP.
---@field mask string mask of the network.

---@class MiioDeviceInfo:table Device information.
---
---@field model string Model, format: "{mfg}.{product}.{submodel}".
---@field mac string MAC address.
---@field fw_ver string Firmware version.
---@field hw_ver string Hardware version.
---@field netif MiioDeviceNetIf Network interface.

---Create a device object.
---@param done fun(self: MiioDevice, ...) Callback will be called after the device is created.
---@param timeout integer Timeout period (in milliseconds).
---@param addr string Device address.
---@param token string Device token.
---@return MiioDevice|nil obj Device object.
function device.create(done, timeout, addr, token, ...)
    assert(type(done) == "function")
    assert(timeout > 0, "timeout must be greater then 0")
    assert(type(addr) == "string")
    assert(type(token) == "string")
    assert(#token == 16)

    ---@class MiioDevice:table Device object.
    local o = {
        done = done,
        args = {...},
        pcb = nil, ---@type MiioPcb
        info = nil, ---@type MiioDeviceInfo|nil
        timeout = timeout,
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

    ---Dispatch the requests.
    ---@param self MiioDevice
    local function dispatch(self)
        if self.state ~= "IDLE" then
            return
        end
        local que = self.cmdQue
        if que.first ~= que.last then
            self.pcb:request(function (err, result, self, respCb, ...)
                respCb(err, result, ...)
                self.state = "IDLE"
                dispatch(self)
            end, self.timeout, deque(que))
            self.state = "BUSY"
        end
    end

    local function handleDoneCb(self)
        self.done(self, table.unpack(self.args))
        self.done = nil
        self.arg = nil
    end

    o.scanCtx = protocol.scan(function (addr, devid, stamp, self, token)
        self.scanTimer:cancel()
        self.scanTimer = nil
        self.scanCtx = nil
        local pcb = protocol.create(addr, devid, token, stamp)
        if not pcb then
            logger:error("Failed to create PCB.")
            handleDoneCb(self)
            return
        end
        self.pcb = pcb
        self.state = "IDLE"
        self:request(function (err, result, self)
            self.info = result
            handleDoneCb(self)
        end, "miIO.info", nil, self)
    end, addr, o, token)
    if not o.scanCtx then
        logger:error("Failed to start scanning.")
        return nil
    end

    o.scanTimer = timer.create()
    o.scanTimer:start(timeout, function (self)
        logger:error("Scan timeout.")
        self.scanCtx:stop()
        self.scanCtx = nil
        self.timer = nil
        handleDoneCb(self)
    end, o)

    ---Start a request and ``respCb`` will be called when a response is received.
    ---@param respCb fun(err: MiioError|string|nil, result: any, ...) Response callback.
    ---@param method string The request method.
    ---@param params? table Array of parameters.
    function o:request(respCb, method, params, ...)
        assert(type(respCb) == "function")

        enque(self.cmdQue, method, params, self, respCb, ...)
        dispatch(self)
    end

    return o
end

return device
