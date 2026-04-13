local core = require "core"

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
