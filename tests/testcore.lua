local core = require "core"
local floor = math.floor

local function spawn(fn, ...)
    core.createTimer(fn, ...):start(0)
end

-- Tests repeated timer callbacks with a userdata argument.
do
    local mq = core.createMQ(1)
    local done = core.createMQ(1)
    local timer
    local count = 0

    timer = core.createTimer(function (queue)
        count = count + 1
        queue:send(count)
        assert(queue:recv() == count)
        if count == 16 then
            done:send(true)
            return
        end
        timer:start(0)
    end, mq)

    timer:start(0)
    assert(done:recv() == true)
end

local function close_with_userdata_error()
    return setmetatable({}, {
        __close = function ()
            error(core.createMQ(1))
        end
    })
end

-- Tests that an errored coroutine does not pollute the coroutine pool.
do
    local ran = false
    local next_timer

    next_timer = core.createTimer(function ()
        ran = true
    end)

    local timer = core.createTimer(function ()
        local _ <close> = close_with_userdata_error()
        next_timer:start(0)
    end)

    timer:start(0)
    core.sleep(20)
    assert(ran == true)
end

-- Tests recvUntil times out without consuming future messages.
do
    local mq = core.createMQ(1)
    local ok, err = mq:recvUntil(floor(core.time()))
    assert(ok == false)
    assert(err == "timeout")

    core.createTimer(function(queue)
        queue:send("late")
    end, mq):start(20)

    ok, err = mq:recvUntil(floor(core.time()) + 5)
    assert(ok == false)
    assert(err == "timeout")
    local success, value = mq:recvUntil(floor(core.time()) + 100)
    assert(success == true)
    assert(value == "late")
end

-- Tests recvUntil receives queued messages before the deadline.
do
    local mq = core.createMQ(1)

    core.createTimer(function(queue)
        queue:send("ok", 42)
    end, mq):start(20)

    local ok, a, b = mq:recvUntil(floor(core.time()) + 100)
    assert(ok == true)
    assert(a == "ok")
    assert(b == 42)
end

-- Tests recvUntil returns queued messages immediately.
do
    local mq = core.createMQ(2)

    mq:send("ready", 7)

    local ok, a, b = mq:recvUntil(floor(core.time()) + 100)
    assert(ok == true)
    assert(a == "ready")
    assert(b == 7)
end

-- Tests a timed out waiter is removed without affecting other waiters.
do
    local mq = core.createMQ(1)
    local done = core.createMQ(2)

    spawn(function(queue, out)
        local ok, err = queue:recvUntil(floor(core.time()) + 10)
        out:send("short", ok, err)
    end, mq, done)

    spawn(function(queue, out)
        local ok, value = queue:recvUntil(floor(core.time()) + 100)
        out:send("long", ok, value)
    end, mq, done)

    core.createTimer(function(queue)
        queue:send("payload")
    end, mq):start(30)

    local tag1, ok1, value1 = done:recv()
    local tag2, ok2, value2 = done:recv()
    local results = {
        [tag1] = {ok1, value1},
        [tag2] = {ok2, value2},
    }

    assert(results.short[1] == false)
    assert(results.short[2] == "timeout")
    assert(results.long[1] == true)
    assert(results.long[2] == "payload")
end
