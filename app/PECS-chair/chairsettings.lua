local Settings = {}

storm.n.set_occupancy_mode(storm.n.ENABLE)
storm.n.set_heater_mode(storm.n.BOTTOM_HEATER, storm.n.ENABLE)
storm.n.set_heater_mode(storm.n.BACK_HEATER, storm.n.ENABLE)
storm.n.set_fan_mode(storm.n.ENABLE)
storm.n.set_temp_mode(storm.n.ENABLE)

storm.n.set_heater_state(storm.n.BOTTOM_HEATER, storm.n.OFF)
storm.n.set_heater_state(storm.n.BACK_HEATER, storm.n.OFF)
storm.n.set_fan_state(storm.n.BOTTOM_FAN, storm.n.OFF)
storm.n.set_fan_state(storm.n.BACK_FAN, storm.n.OFF)

heaterSettings = {[storm.n.BOTTOM_HEATER] = 0, [storm.n.BACK_HEATER] = 0}
fanSettings = {[storm.n.BOTTOM_FAN] = 0, [storm.n.BACK_FAN] = 0}
fans = {storm.n.BOTTOM_FAN, storm.n.BACK_FAN}

heaters = {storm.n.BOTTOM_HEATER, storm.n.BACK_HEATER}

-- SETTING is from 0 to 100
function setHeater(heater, setting)
   heaterSettings[heater] = setting
end

-- SETTING is from 0 to 100
function setFan(fan, setting)
   fanSettings[fan] = setting
   if storm.n.check_occupancy() then
         storm.n.set_fan_state(fan, storm.n.quantize_fan(setting))
   end
end

storm.n.flash_init()
storm.n.flash_get_saved_settings(function (backh, bottomh, backf, bottomf, timediff)
    setHeater(storm.n.BACK_HEATER, backh)
    setHeater(storm.n.BOTTOM_HEATER, bottomh)
    setFan(storm.n.BACK_FAN, backf)
    setFan(storm.n.BOTTOM_FAN, bottomf)
    storm.n.set_time_diff(timediff)
    storm.n.flash_save_settings(0, 0, 0, 0, 0, function () -- So it turns off if the user taps the screen
        storm.n.flash_write_log(nil, 0, 0, 0, 0, 0, 0, 0, true, function () print("Logging reboot") end)
    end)
end)

function modulateHeater(heater)
    storm.os.invokePeriodically(storm.os.SECOND, function ()
        local setting = 10 * heaterSettings[heater] * storm.os.MILLISECOND
        if not storm.n.check_occupancy() then
            setting = 0
        end
        if setting > 0 then
            storm.n.set_heater_state(heater, storm.n.ON)
        end
        if setting < storm.os.SECOND then
            storm.os.invokeLater(setting, storm.n.set_heater_state, heater, storm.n.OFF)
        end
    end)
end

for _, heater in pairs(heaters) do
    modulateHeater(heater)
end

local sendHandler = function (message) if message ~= nil then print("Success!") else print("15.4 Failed") end end

rnqcl = storm.n.RNQClient:new(30000)
function updateSMAP()
   -- Update sMAP
   local temp
   local humidity
   temp, humidity = storm.n.get_temp_humidity(storm.n.CELSIUS)
   local pyld = { storm.os.nodeid(), storm.n.check_occupancy(), heaterSettings[storm.n.BACK_HEATER], heaterSettings[storm.n.BOTTOM_HEATER], fanSettings[storm.n.BACK_FAN], fanSettings[storm.n.BOTTOM_FAN], temp, humidity }
   
   send_time = storm.n.get_time_always()
   
   -- Update the phone
   local occ = 0
   if pyld[2] then
      occ = 1
   end
   local strpyld = storm.n.pack_string(pyld[3], pyld[4], pyld[5], pyld[6], occ, temp, humidity)
   storm.n.bl_PECS_send(strpyld)
   
   -- Log to Flash
   storm.n.flash_write_log(storm.n.get_time(), pyld[3], pyld[4], pyld[5], pyld[6], temp, humidity, occ, false,
       function ()
           print("Logged")
           rnqcl:sendMessage(pyld, "ff02::1", 30002, 175, 100 * storm.os.MILLISECOND, nil, time_sync_handler)
       end)
   print("Updated")
end

storm.os.invokePeriodically(20 * storm.os.SECOND, updateSMAP)

local last_occupancy_state = false
storm.os.invokePeriodically(
   storm.os.SECOND,
   function ()
      local current_state = storm.n.check_occupancy()
      if current_state and not last_occupancy_state then
         for i = 1,#fans do
            fan = fans[i]
            storm.n.set_fan_state(fan, storm.n.quantize_fan(fanSettings[fan]))
         end
      elseif not current_state and last_occupancy_state then
         for i = 1,#fans do
            fan = fans[i]
            storm.n.set_fan_state(fan, storm.n.OFF)
         end
      end
      last_occupancy_state = current_state
   end
)

Settings.setHeater = setHeater
Settings.setFan = setFan
Settings.updateSMAP = updateSMAP

return Settings
