-- RUNS ON PECS CHAIR BOARD

-- Turn off the backlight
BL_CTL = storm.io.D5
storm.io.set_mode(storm.io.OUTPUT, BL_CTL)
storm.io.set(0, BL_CTL)

storm.n.chairsettings_init()
storm.n.periodically_ping_bl()

data_ip = "2001:470:1f04:5f2::2" -- Where to send the data. ff02::1 to use firestorm proxy

-- Store saved settings in flash and reset
--[[storm.os.invokePeriodically(1213 * storm.os.SECOND, function ()
    local h = heaterSettings
    local f = fanSettings
    local timediff = storm.n.get_time()
    if timediff == nil then
        timediff = 0
    else
        timediff = timediff + 3
    end
    storm.n.flash_save_settings(h[storm.n.BACK_HEATER],
                          h[storm.n.BOTTOM_HEATER],
                          f[storm.n.BACK_FAN],
                          f[storm.n.BOTTOM_FAN],
                          timediff,
                          storm.os.reset)
end)]]

storm.n.enable_reset()

server = storm.n.RNQServer:new(60001, storm.n.actuation_handler)

-- Synchronize time with firestorm
function time_sync_handler(msg)
    if msg ~= nil and msg.time ~= nil then
        local recv_time = storm.n.get_time_always()
        local diff = storm.n.compute_time_diff(send_time, msg.time, msg.time, recv_time)
        print("Calculated diff " .. diff)
        if storm.n.set_time_diff(diff) then
            storm.n.autosender_register_time_sync()
        end
    end
end

empty = {}
time_sync = storm.n.RNQClient:new(60003)
function sync_time()
    send_time = storm.n.get_time_always()
    print("asking for time")
    time_sync:sendMessage(empty,
                          data_ip,
                          38002,
                          250,
                          200 * storm.os.MILLISECOND,
                          nil,
                          time_sync_handler)
end
storm.os.invokePeriodically(60 * storm.os.SECOND, sync_time)
sync_time()

storm.n.bl_PECS_init()
storm.n.bl_PECS_clear_recv_buf()

function handle_bl_msg(bytes)
    b1, b2, b3, b4, b5 = storm.n.interpret_string(bytes)
    print("using bl handler")
    storm.n.bl_handler(b1, b2, b3, b4, b5, bytes)
    print("Got", b1, b2, b3, b4, b5)
    storm.n.bl_PECS_receive_cb(5, handle_bl_msg)
end

storm.os.invokePeriodically(1 * storm.os.SECOND, function ()
    collectgarbage("collect")
    print("Using " .. gcinfo())
    print("Bytes " .. storm.n.gcbytes())
end)

storm.n.bl_PECS_receive_cb(5, handle_bl_msg)

while true do
    storm.os.wait_callback()
end
