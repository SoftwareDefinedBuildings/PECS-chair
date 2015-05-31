require "cord"

cord.new(function ()
    print()
    print("Resetting log...")
    storm.n.flash_init()
    cord.await(storm.n.flash_reset_log)
    cord.await(storm.os.invokeLater, storm.os.SECOND)
    local new_sp = cord.await(storm.n.flash_read_sp)
    print("New sp = " .. new_sp)
end)

cord.enter_loop()
