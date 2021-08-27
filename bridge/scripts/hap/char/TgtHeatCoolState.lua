local tgtHeatCoolState = {
    value = {
        HeatOrCool= 0,
        Heat = 1,
        Cool = 2
    }
}

function tgtHeatCoolState.new(iid, read, write)
    return {
        format = "UInt8",
        iid = iid,
        type = "TargetHeaterCoolerState",
        props = {
            readable = true,
            writable = true,
            supportsEventNotification = true,
            requiresTimedWrite = true,
            ble = {
                supportsBroadcastNotification = true,
                supportsDisconnectedNotification = true
            }
        },
        units = "None",
        constraints = {
            minVal = 0,
            maxVal = 2,
            stepVal = 1
        },
        cbs = {
            read = read,
            write = write
        }
    }
end

return tgtHeatCoolState
