require "cord"
cord.new(function ()
    print() -- Add a newline so it's easier to read
    local log_length = cord.await(storm.n.flash_get_log_size)
    local final_log_index = log_length - 1
    print("=============== START LOG ===============")
    print("Index", "Time", "", "Back Heat", "Bottom Heat", "Back Fan", "Bottom Fan", "Temperature", "Humidity", "Occupancy", "Rebooted")
    for i = 0, final_log_index do
        time, backh, bottomh, backf, bottomf, temp, hum, occ, rebooted = cord.await(storm.n.flash_read_log, i)
        if time == nil then
            time = "Unknown Time"
        end
        print(i, time, backh, "", bottomh, "", backf, "", bottomf, "", temp, "", hum, "", occ, "", rebooted)
    end
    print("================ END LOG ================")
end)

cord.enter_loop()
