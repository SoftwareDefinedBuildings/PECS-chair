require "cord"
sh = require("stormsh")

shell_storm_pm = "2001:470:1f04:5f2::2"

send_sock = storm.net.udpsocket(
  3000,
  function(payload, from, port)
    print("Received payload", payload)
    print("ip ", from, "port", port)
  end
)

function update_smap()
  occupancy = storm.n.check_occupancy()
  payload = {
    "macaddr": "12345",
    "occupancy": occupancy
  }
  storm.net.sendto(sendsock, storm.mp.pack(payload), shell_storm_pm, 39000)
end

storm.os.invokePeriodically(10*storm.os.SECOND, update_smap)


sh.start()
cord.enter_loop()
