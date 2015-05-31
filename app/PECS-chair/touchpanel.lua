require "cord"
sh = require "stormsh"

CHX = 0x90
CHY = 0xd0

storm.spi.init(0, 500000)
storm.spi.setcs(1)

function tp_read()
   tx = storm.array.create(2, storm.array.UINT8)
   tx:set(1, 0x00)
   tx:set(2, 0x00)
   rx = storm.array.create(2, storm.array.UINT8)
   cord.await(storm.spi.xfer, tx, rx)
   return rx
end

function tp_wr_cmd(cmd)
   tx = storm.array.create(1, storm.array.UINT8)
   tx:set(1, cmd)
   rx = storm.array.create(1, storm.array.UINT8)
   cord.await(storm.spi.xfer, tx, rx)
   return rx
end

function tp_read_x()
   storm.spi.setcs(0)
   tp_wr_cmd(CHX)
   i = tp_read()
   storm.spi.setcs(1)
   return i
end

function tp_read_y()
   storm.spi.setcs(0)
   tp_wr_cmd(CHY)
   i = tp_read()
   storm.spi.setcs(1)
   return i
end

function tp_get_x_y()
   x = tp_read_x()
   y = tp_read_y()
   return {x, y}
end

TP_IRQ = storm.io.D7
HMSOFT_RESET = storm.io.D6

storm.io.set_mode(storm.io.INPUT, TP_IRQ)
storm.io.set_mode(storm.io.OUTPUT, HMSOFT_RESET)
storm.io.set(1, HMSOFT_RESET)


count = 0

-- Uses watch single recursively to prevent multiple
-- callbacks firing at once.  Prevents concurrency
-- issues with SPI bus.  Only one touch event is
-- handled at a time and while it is being hanndled
-- any new events are ignored.
function handle_change()
    cord.new(
        function ()
           print("TP_IRQ falling")
           count = count + 1
           if count == 4 then
              print("Resetting")
              -- Turn off HMSOFT
              storm.io.set(0, HMSOFT_RESET)
              storm.os.invokeLater(
                 storm.os.MILLISECOND * 2000,
                 function ()
                    -- Turn on HMSOFT
                    storm.io.set(1, HMSOFT_RESET)
                    -- Reset storm
                    storm.os.reset()
                 end
              )
           end
           -- Call this to read the touchpanel values over SPI
           -- currently doesn't work, returns 0
           -- vals = tp_get_x_y()
           -- print("x1", vals[1]:get(1))
           -- print("x2", vals[1]:get(2))
           -- print("y1", vals[2]:get(1))
           -- print("y2", vals[2]:get(2))
           storm.io.watch_single(
               storm.io.FALLING,
               storm.io.D7,
               handle_change
           )
        end
    )
end

storm.io.watch_single(
   storm.io.FALLING,
   storm.io.D7,
   handle_change
)

storm.os.invokePeriodically(
   storm.os.SECOND,
   function ()
      count = 0
   end
)


sh.start()

cord.enter_loop()
