local udp = require "pal.net.udp"

local protocol = {}

---
--- Packet format
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

---Pack a packet.
---@param unknown integer Unknown: 32-bit.
---@param did integer Device ID: 32-bit.
---@param stamp integer Stamp: 32 bit unsigned int.
---@param checksum integer[] A array of 32-bit integers. MD5 checksnum: 128-bit.
---@param opt? string Optional variable-sized data.
function protocol.pack(unknown, did, stamp, checksum, opt)
    assert(#checksum == 4)
    local len = 32
    local fmt = ">I2>I2>I4>I4>I4>I4>I4>I4>I4"
    if opt then
        len = len + #opt
        fmt = fmt .. "z"
    end
    return string.pack(fmt, 0x2131, len, unknown, did, stamp,
        checksum[1], checksum[2], checksum[3], checksum[4], opt)
end

function protocol.packHello()
    return protocol.pack(
        0xffffffff,
        0xffffffff,
        0xffffffff,
        { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
    )
end

---Scan for devices in the network.
---@param addr string
---@param cb fun(device: table)
---@param timeout integer
function protocol.scan(addr, cb, timeout)
    local handle = udp.open("inet")
    if not addr then
        handle:enableBroadcast()
    end
    handle:sendto(protocol.packHello(), addr or "255.255.255.255", 54321)
    handle:setRecvCb(function (data, from_addr, from_port)

    end)
end

return protocol
