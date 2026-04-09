assert(core.isYieldable() == true)

---Test Future resolve before wait.
do
    local future = core.createFuture()
    assert(future:state() == "pending")

    future:resolve(1, nil, 3)
    assert(future:state() == "resolved")

    local ok, a, b, c = future:wait()
    assert(ok == true)
    assert(a == 1)
    assert(b == nil)
    assert(c == 3)

    local success = pcall(future.resolve, future, 2)
    assert(success == false)
end

---Test Future reject after asynchronous completion.
do
    local future = core.createFuture()
    core.createTimer(function ()
        future:reject("boom", nil)
    end):start(0)

    local ok, err, extra = future:wait()
    assert(ok == false)
    assert(err == "boom")
    assert(extra == nil)
    assert(future:state() == "rejected")
end

---Test multiple waiters observing the same future completion.
do
    local future = core.createFuture()
    local queue = core.createQueue(2)

    core.createTimer(function ()
        local ok, value = future:wait()
        queue:send("a", ok, value)
    end):start(0)
    core.createTimer(function ()
        local ok, value = future:wait()
        queue:send("b", ok, value)
    end):start(0)
    core.createTimer(function ()
        future:resolve(42)
    end):start(0)

    local seen = {}
    for _ = 1, 2 do
        local tag, ok, value = queue:recv()
        assert(ok == true)
        assert(value == 42)
        seen[tag] = true
    end
    assert(seen.a == true)
    assert(seen.b == true)
end

---Test Event sticky set/clear semantics and broadcast wake-up.
do
    local event = core.createEvent()
    local queue = core.createQueue(2)
    assert(event:isSet() == false)

    core.createTimer(function ()
        event:wait()
        queue:send("a")
    end):start(0)
    core.createTimer(function ()
        event:wait()
        queue:send("b")
    end):start(0)
    core.createTimer(function ()
        event:set()
    end):start(0)

    local seen = {}
    seen[queue:recv()] = true
    seen[queue:recv()] = true
    assert(seen.a == true)
    assert(seen.b == true)

    assert(event:isSet() == true)
    event:wait()
    event:clear()
    assert(event:isSet() == false)
end

---Test Queue buffering, nil preservation and full-queue failure.
do
    local queue = core.createQueue(2)
    assert(queue:cap() == 2)
    assert(queue:len() == 0)

    queue:send(1, nil, 3)
    queue:send("x")
    assert(queue:len() == 2)

    local a, b, c = queue:recv()
    assert(a == 1)
    assert(b == nil)
    assert(c == 3)
    assert(queue:len() == 1)

    assert(queue:recv() == "x")
    assert(queue:len() == 0)

    queue:send("m1")
    queue:send("m2")
    local success = pcall(queue.send, queue, "m3")
    assert(success == false)
end

---Test Queue direct hand-off to a waiting receiver.
do
    local queue = core.createQueue(1)
    core.createTimer(function ()
        core.sleep(0)
        queue:send("hello", nil)
    end):start(0)

    local msg, extra = queue:recv()
    assert(msg == "hello")
    assert(extra == nil)
    assert(queue:len() == 0)
end
