require "cord"
BL_NAME = require "name"

cord.new(function ()
    BL_NAME.set_name()
    
    storm.n.bl_PECS_init()
    storm.n.bl_PECS_receive_cb_init()
    
    storm.n.bl_PECS_send("AT+RESET")
    print("> AT+RESET")
    local reset = cord.await(storm.n.bl_PECS_receive_cb, 8)
    print("< " .. reset)
    
    storm.n.bl_PECS_send("AT+NAME" .. bl_name)
    print("> AT+NAME" .. bl_name)
    local nameconf = cord.await(storm.n.bl_PECS_receive_cb, 14)
    print("< " .. nameconf)
    
    storm.n.bl_PECS_send("AT+RESET")
    print("> AT+RESET")
    reset = cord.await(storm.n.bl_PECS_receive_cb, 8)
    print("< " .. reset)
end)

cord.enter_loop()
