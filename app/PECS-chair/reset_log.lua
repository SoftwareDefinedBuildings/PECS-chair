require "cord"

cord.new(function ()
    print()
    print("Resetting log...")
    storm.n.flash_init()
    cord.await(storm.n.flash_write_sp, 768)
    cord.await(storm.n.flash_write_bo, 0)
    cord.await(storm.n.flash_write_cp, 768)
    local new_sp = cord.await(storm.n.flash_read_sp)
    print("New sp = " .. new_sp)
    local new_bo = cord.await(storm.n.flash_read_bo)
    print("New bo = " .. new_bo)
    local new_cp = cord.await(storm.n.flash_read_cp)
    print("New cp = " .. new_cp)
end)

cord.enter_loop()
