RNQC = storm.n.RNQClient
RNQS = storm.n.RNQServer

rnqcl = RNQC:new(60000)

--shell_ip = "2001:470:1f04:5f2::2"
--proj_ip = "2001:470:66:3f9::2"
cbe_ip = "2001:470:39:375::2"

storm.os.invokeLater(5 * storm.os.MINUTE, function () storm.os.reset() end)

storm.os.invokePeriodically(10 * storm.os.SECOND, function ()
    collectgarbage("collect")
    print("Using " .. gcinfo())
    print("Bytes " .. storm.n.gcbytes())
end)

server_ip = cbe_ip
ok = {["rv"] = "ok"}
function sendActuationMessage(payload, srcip, srcport)
   local toIP = payload["toIP"]
   payload["toIP"] = nil
   print("Actuating " .. toIP)
   rnqcl:sendMessage(payload,
                     toIP,
                     60004,
                     180,
                     100 * storm.os.MILLISECOND,
                     function ()
                        print("trying")
                     end,
                     function (payload, address, port)
                        if payload == nil then
                           print("Send FAILS.")
                        else
                           print("Response received.")
                        end
                     end)
    return ok
end

from_server = RNQS:new(60001, sendActuationMessage)
to_server = RNQC:new(30001)

chairForwarder = RNQS:new(30002,
                          function (payload, ip, port)
                             print(ip)
                     
                             local msg = storm.mp.pack(payload)
                     
                             to_server:sendMessage(payload,
                                                   server_ip,
                                                   38003,
                                                   100,
                                                   100 * storm.os.MILLISECOND,
                                                   nil,
                                                   function (msg)
                                                      if msg ~= nil then
                                                         print("Success")
                                                      end
                                                   end)
                             return {["time"] = storm.n.get_time()}  -- Allow chairs to synchronize time with firestorm
                          end)
                                 
-- Synchronize time with server every 20 seconds
time_sync = RNQC:new(30003)
empty = {}
function synctime()
    local send_time = storm.n.get_time_always()
    print("asking for time")
    time_sync:sendMessage(empty,
                          server_ip,
                          38002,
                          100,
                          100 * storm.os.MILLISECOND,
                          nil,
                          function (msg)
                             if msg ~= nil then
                                 print ("got time message " .. msg.time)
                                 local recv_time = storm.n.get_time_always()
                                 local diff = storm.n.compute_time_diff(send_time, msg.time, msg.time, recv_time)
                                 print("Calculated diff " .. diff)
                                 storm.n.set_time_diff(diff)
                             end
                          end)
end
synctime()
storm.os.invokePeriodically(20 * storm.os.SECOND, synctime)
    
-- Allow chairs to synchronize time with firestorm
--synchronizer = RNQS:new(30004, function () print("Got synchronization request") return {["time"] = storm.n.get_time()} end)

while true do
    storm.os.wait_callback()
end
