local udp = require "pal.net.udp"
local timer = require "pal.timer"

local protocol = {}
local logger = log.getLogger("miio.protocol")

---
--- Message format.
---
--- 0                   1                   2                   3
--- 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
--- +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--- | Magic number = 0x2131         | Packet Length (incl. header)  |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Unknown                                                       |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Device ID ("did")                                             |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | Stamp                                                         |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | MD5 checksum                                                  |
--- | ... or Device Token in response to the "Hello" packet         |
--- |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
--- | optional variable-sized data (encrypted)                      |
--- |...............................................................|
---
---                 Mi Home Binary Protocol header
---        Note that one tick mark represents one bit position.
---
---  Magic number: 16 bits
---      Always 0x2131
---
---  Packet length: 16 bits unsigned int
---      Length in bytes of the whole packet, including the header.
---
---  Unknown: 32 bits
---      This value is always 0,
---      except in the "Hello" packet, when it's 0xFFFFFFFF
---
---  Device ID: 32 bits
---      Unique number. Possibly derived from the MAC address.
---      except in the "Hello" packet, when it's 0xFFFFFFFF
---
---  Stamp: 32 bit unsigned int
---      continously increasing counter
---
---  MD5 checksum:
---      calculated for the whole packet including the MD5 field itself,
---      which must be initialized with 0.
---
---      In the special case of the response to the "Hello" packet,
---      this field contains the 128-bit device token instead.
---
---  optional variable-sized data:
---      encrypted with AES-128: see below.
---      length = packet_length - 0x20
---
---@class ProtocolMessage
---
---@field unknown integer
---@field did integer
---@field stamp integer
---@field checksum string
---@field data string

---Pack a message to a binary package.
---@param unknown integer Unknown: 32-bit.
---@param did integer Device ID: 32-bit.
---@param stamp integer Stamp: 32 bit unsigned int.
---@param checksum string MD5 checksnum: 128-bit.
---@param data? string Optional variable-sized data.
---@return string package
function protocol.pack(unknown, did, stamp, checksum, data)
    assert(#checksum == 16)
    local len = 32
    local fmt = ">I2>I2>I4>I4>I4c" .. #checksum
    if data then
        len = len + #data
        fmt = fmt .. "c" .. #data
    end
    return string.pack(fmt, 0x2131, len, unknown, did, stamp, checksum, data)
end

---Unpack a message from a binary package.
---@param package string A binary package.
---@return ProtocolMessage|nil message
function protocol.unpack(package)
    if string.unpack(">I2", package, 1) ~= 0x2131 then
        return nil
    end

    local len = string.unpack(">I2", package, 3)
    if len < 32 or len ~= #package then
        return nil
    end

    return {
        unknown = string.unpack(">I4", package, 5),
        did = string.unpack(">I4", package, 9),
        stamp = string.unpack(">I4", package, 13),
        checksum = string.unpack("c16", package, 17),
        data = string.unpack("c" .. len - 32, package, 33)
    }
end

---Pack a hello package.
---@return string package
function protocol.packHello()
    return protocol.pack(
        0xffffffff,
        0xffffffff,
        0xffffffff,
        string.pack(">I4>I4>I4>I4", 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff)
    )
end

---Scan for devices in the network.
---@param cb fun(addr: string, port: integer, devId: integer) Function call when the device is scaned.
---@param timeout integer Timeout period (in seconds).
---@param addr? string Target Address.
---@return boolean status true on success, false on failure.
function protocol.scan(cb, timeout, addr)
    assert(type(cb) == "function")
    assert(timeout > 0)

    local c = {}
    local h = udp.open("inet")
    if not h then
        logger:error("Fail to open a UDP handle.")
        return false
    end
    if not addr then
        if h:enableBroadcast() == false then
            logger:error("Failed to enable UDP broadcast.")
            h:close()
            return false
        end
    end
    local t = timer.create(timeout * 1000, function(context)
        logger:debug("Scan done.")
    end, c)
    if not t then
        logger:error("Failed to create a timer.")
        h:close()
        return false
    end
    if not h:sendto(protocol.packHello(), addr or "255.255.255.255", 54321) then
        logger:error("Failed to send hello message.")
        h:close()
        t:destroy()
        return false
    end
    h:setRecvCb(function (data, from_addr, from_port, cb)
        local m = protocol.unpack(data)
        if m and m.unknown == 0 and m.data == nil then
            cb(from_addr, from_port, m.did)
        end
    end, cb)
    c.handle = h
    c.timer = t
    return true
end

return protocol
