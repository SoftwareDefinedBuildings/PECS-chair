#include "chairsettings.h"

/* The initialization function does the work needed to set up physical chair actuation
   and provide and API to control the chair. It also begins these processes and repeatedly
   sends the current state over both bluetooth and 802.15.4 and logs the data to flash
   memory.
   
   It corresponds to the following Lua code:
   
storm.n.set_occupancy_mode(storm.n.ENABLE)
storm.n.set_heater_mode(storm.n.BOTTOM_HEATER, storm.n.ENABLE)
storm.n.set_heater_mode(storm.n.BACK_HEATER, storm.n.ENABLE)
storm.n.set_fan_mode(storm.n.ENABLE)
storm.n.set_temp_mode(storm.n.ENABLE)

storm.n.set_heater_state(storm.n.BOTTOM_HEATER, storm.n.OFF)
storm.n.set_heater_state(storm.n.BACK_HEATER, storm.n.OFF)
storm.n.set_fan_state(storm.n.BOTTOM_FAN, storm.n.OFF)
storm.n.set_fan_state(storm.n.BACK_FAN, storm.n.OFF)

__rnqcl = storm.n.RNQClient:new(30000)

storm.n.flash_init()
storm.n.enqueue_flash_task(storm.n.flash_read_sp, storm.n.autosender_init)
storm.n.enqueue_flash_task(storm.n.flash_write_log, nil, 0, 0, 0, 0, 0, 0, 0, true, function () print("Logging reboot") end)
storm.n.enqueue_flash_task(storm.n.flash_get_saved_settings, function (backh, bottomh, backf, bottomf, timediff)
    storm.n.set_heater(storm.n.BACK_HEATER, backh)
    storm.n.set_heater(storm.n.BOTTOM_HEATER, bottomh)
    storm.n.set_fan(storm.n.BACK_FAN, backf)
    storm.n.set_fan(storm.n.BOTTOM_FAN, bottomf)
    storm.n.set_time_diff(timediff)
end)
storm.n.enqueue_flash_task(storm.n.flash_save_settings, 0, 0, 0, 0, 0, function () print("Cleared saved settings") end) -- So it turns off if the user taps the screen

storm.os.invokePeriodically(20 * storm.os.SECOND, storm.n.update_server) -- For synchronization
storm.n.modulate_heaters() -- Already a derivative
storm.n.modulate_fans() -- Already a derivative

*/

int confirm_reboot(lua_State* L);
int set_curr_state(lua_State* L);
int confirm_clear(lua_State* L);

int chairsettings_init(lua_State* L) {
    lua_pushlightfunction(L, set_occupancy_mode);
    lua_pushnumber(L, ENABLE);
    lua_call(L, 1, 0);
    
    lua_pushlightfunction(L, set_heater_mode);
    lua_pushnumber(L, BOTTOM_HEATER);
    lua_pushnumber(L, ENABLE);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_heater_mode);
    lua_pushnumber(L, BACK_HEATER);
    lua_pushnumber(L, ENABLE);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_fan_mode);
    lua_pushnumber(L, ENABLE);
    lua_call(L, 1, 0);
    
    lua_pushlightfunction(L, set_temp_mode);
    lua_pushnumber(L, ENABLE);
    lua_call(L, 1, 0);
    
    lua_pushlightfunction(L, set_heater_state);
    lua_pushnumber(L, BOTTOM_HEATER);
    lua_pushnumber(L, OFF);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_heater_state);
    lua_pushnumber(L, BACK_HEATER);
    lua_pushnumber(L, OFF);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_fan_state);
    lua_pushnumber(L, BOTTOM_FAN);
    lua_pushnumber(L, OFF);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_fan_state);
    lua_pushnumber(L, BACK_FAN);
    lua_pushnumber(L, OFF);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, rnqclient_new);
    lua_pushnil(L);
    lua_pushnumber(L, 30000);
    lua_call(L, 2, 1);
    lua_setglobal(L, "__rnqcl");
    
    lua_pushlightfunction(L, flash_init);
    lua_call(L, 0, 0);
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, read_sp);
    lua_pushlightfunction(L, init_autosender);
    lua_call(L, 2, 2);
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, write_log_entry);
    lua_pushnil(L);
    int i;
    for (i = 0; i < 7; i++) {
        lua_pushnumber(L, 0);
    }
    lua_pushboolean(L, 1);
    lua_pushlightfunction(L, confirm_reboot);
    lua_call(L, 11, 0);
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, get_saved_settings);
    lua_pushlightfunction(L, set_curr_state);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, save_settings);
    for (i = 0; i < 5; i++) {
        lua_pushnumber(L, 0);
    }
    lua_pushlightfunction(L, confirm_clear);
    lua_call(L, 7, 0);
    
    lua_pushlightfunction(L, libstorm_os_invoke_periodically);
    lua_pushnumber(L, 20 * SECOND_TICKS);
    lua_pushlightfunction(L, update_server);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, modulate_heaters);
    lua_call(L, 0, 0);
    
    lua_pushlightfunction(L, modulate_fans);
    lua_call(L, 0, 0);
    
    return 0;
}

int confirm_reboot(lua_State* L) {
    printf("Logging reboot\n");
    return 0;
}

int set_curr_state(lua_State* L) {
    int i;
    for (i = 1; i <= 5; i++) {
        luaL_checkint(L, i); // check that all of the arguments are integers
    }

    lua_pushlightfunction(L, set_heater);
    lua_pushnumber(L, BACK_HEATER);
    lua_pushvalue(L, 1);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_heater);
    lua_pushnumber(L, BOTTOM_HEATER);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_fan);
    lua_pushnumber(L, BACK_FAN);
    lua_pushvalue(L, 3);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_fan);
    lua_pushnumber(L, BOTTOM_FAN);
    lua_pushvalue(L, 4);
    lua_call(L, 2, 0);
    
    lua_pushlightfunction(L, set_time_diff);
    lua_pushvalue(L, 5);
    lua_call(L, 1, 0);
    
    return 0;
}

int confirm_clear(lua_State* L) {
    printf("Cleared saved settings\n");
    return 0;
}

/* These global variables store the current settings, and correspond to the following Lua code

heaterSettings = {[storm.n.BOTTOM_HEATER] = 0, [storm.n.BACK_HEATER] = 0}
fanSettings = {[storm.n.BOTTOM_FAN] = 0, [storm.n.BACK_FAN] = 0}
fans = {storm.n.BOTTOM_FAN, storm.n.BACK_FAN}
heaters = {storm.n.BOTTOM_HEATER, storm.n.BACK_HEATER}

*/

uint8_t heaterSettings[2] = {0, 0};
uint8_t fanSettings[2] = {0, 0};
// heaters array: this is taken care of since NUM_HEATERS = 2
// fans array: this is taken care of since NUM_FANS = 2

/* These two functions correspond to the following Lua code.

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

*/

int set_heater(lua_State* L) {
    int heater = luaL_checkint(L, 1);
    uint8_t setting = (uint8_t) luaL_checkint(L, 2);
    heaterSettings[heater] = setting;
    return 0;
}

int set_fan(lua_State* L) {
    int fan = luaL_checkint(L, 1);
    uint8_t setting = (uint8_t) luaL_checkint(L, 2);
    fanSettings[fan] = setting;
    lua_pushlightfunction(L, check_occupancy);
    lua_call(L, 0, 1);
    if (lua_toboolean(L, -1)) {
        lua_pushlightfunction(L, set_fan_state);
        lua_pushvalue(L, 1);
        lua_pushlightfunction(L, quantize_fan);
        lua_pushvalue(L, 2);
        lua_call(L, 1, 1);
        lua_call(L, 2, 0);
    }
    return 0;
}

int get_heater(lua_State* L) {
    int heater = luaL_checkint(L, 1);
    lua_pushnumber(L, heaterSettings[heater]);
    return 1;
}

int get_fan(lua_State* L) {
    int fan = luaL_checkint(L, 1);
    lua_pushnumber(L, fanSettings[fan]);
    return 1;
}

/* update_server, send_payload, and send_handler corresponds to the following Lua code

sendHandler = function (message) if message ~= nil then print("Success!") else print("15.4 Failed") end end
function updateSMAP()
   -- Update sMAP
   local temp
   local humidity
   temp, humidity = storm.n.get_temp_humidity(storm.n.CELSIUS)
   local pyld = { storm.os.nodeid(), storm.n.check_occupancy(), heaterSettings[storm.n.BACK_HEATER], heaterSettings[storm.n.BOTTOM_HEATER], fanSettings[storm.n.BACK_FAN], fanSettings[storm.n.BOTTOM_FAN], temp, humidity }
   
   -- Update the phone
   local occ = 0
   if pyld[2] then
      occ = 1
   end
   local strpyld = storm.n.pack_string(pyld[3], pyld[4], pyld[5], pyld[6], occ, temp, humidity, pyld[1])
   storm.n.bl_PECS_send(strpyld)
   
   -- Log to Flash
   storm.n.enqueue_flash_task(storm.n.flash_write_log, storm.n.get_time_diff(), pyld[3], pyld[4], pyld[5], pyld[6], temp, humidity, occ, false,
       function ()
           print("Logged")
           rnqcl:empty()
           rnqcl:sendMessage(pyld, data_ip, 38003, 15, storm.os.SECOND, nil, sendHandler)
       end)
   print("Updated")
end

*/

int send_payload(lua_State* L);
int send_handler(lua_State* L);

int update_server(lua_State* L) {
    // Prepare payload to update server
    lua_pushlightfunction(L, lua_get_temp_humidity);
    lua_pushnumber(L, CELSIUS);
    lua_call(L, 1, 2);
    int humidity_index = lua_gettop(L);
    int temp_index = humidity_index - 1;
    lua_pushlightfunction(L, check_occupancy);
    lua_call(L, 0, 1);
    int occ_index = lua_gettop(L);
    lua_createtable(L, 8, 0);
    int pyld_index = lua_gettop(L);
    int i;
    for (i = 1; i <= 8; i++) {
        lua_pushnumber(L, i);
        switch (i) {
            case 1:
                lua_pushlightfunction(L, libstorm_os_getnodeid);
                lua_call(L, 0, 1);
                break;
            case 2:
                lua_pushvalue(L, occ_index);
                break;
            case 3:
                lua_pushnumber(L, heaterSettings[BACK_HEATER]);
                break;
            case 4:
                lua_pushnumber(L, heaterSettings[BOTTOM_HEATER]);
                break;
            case 5:
                lua_pushnumber(L, fanSettings[BACK_FAN]);
                break;
            case 6:
                lua_pushnumber(L, fanSettings[BOTTOM_FAN]);
                break;
            case 7:
                lua_pushvalue(L, temp_index);
                break;
            case 8:
                lua_pushvalue(L, humidity_index);
                break;
        }
        lua_settable(L, pyld_index);
    }
    
    // Update phone
    lua_pushlightfunction(L, bl_PECS_send);
    lua_pushlightfunction(L, pack_string);
    for (i = 3; i <= 6; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, pyld_index);
    }
    int occ = lua_toboolean(L, occ_index); // to convert from boolean to integer
    lua_pushnumber(L, occ);
    lua_pushvalue(L, temp_index);
    lua_pushvalue(L, humidity_index);
    lua_pushnumber(L, 1);
    lua_gettable(L, pyld_index);
    lua_call(L, 8, 1); // call pack_string
    lua_call(L, 1, 0); // call bl_PECS_send
    
    // Log to flash
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, write_log_entry);
    lua_pushlightfunction(L, get_time_diff);
    lua_call(L, 0, 1);
    for (i = 3; i <= 6; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, pyld_index);
    }
    lua_pushvalue(L, temp_index);
    lua_pushvalue(L, humidity_index);
    lua_pushnumber(L, occ);
    lua_pushboolean(L, 0);
    lua_pushvalue(L, pyld_index);
    lua_pushcclosure(L, send_payload, 1);
    lua_call(L, 11, 0);
    printf("Updated\n");
    
    return 0;
}

int send_payload(lua_State* L) {
    printf("Logged\n");
    
    lua_pushlightfunction(L, rnqclient_empty);
    lua_getglobal(L, "__rnqcl");
    lua_call(L, 1, 0);
    
    lua_pushlightfunction(L, rnqclient_sendMessage);
    lua_getglobal(L, "__rnqcl");
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_getglobal(L, "data_ip");
    lua_pushnumber(L, 38003);
    lua_pushnumber(L, 15);
    lua_pushnumber(L, SECOND_TICKS);
    lua_pushnil(L);
    lua_pushlightfunction(L, send_handler);
    lua_call(L, 8, 0);
    
    return 0;
}

int send_handler(lua_State* L) {
    if (lua_isnil(L, 1)) {
        printf("15.4 Failed\n");
    } else {
        printf("Success!\n");
    }
    return 0;
}

/* Modulation of heaters. Corresponds to the following Lua code:

function modulateHeater(heater)
    storm.os.invokePeriodically(storm.os.SECOND, function ()
        local setting = 10 * storm.n.get_heater(heater) * storm.os.MILLISECOND
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
    storm.n.modulate_heater(heater)
end

*/

int do_heater_cycle(lua_State* L);
int modulate_heater(lua_State* L);

int modulate_heaters(lua_State* L) {
    int heater;
    for (heater = 0; heater < NUM_HEATERS; heater++) {
        lua_pushlightfunction(L, modulate_heater);
        lua_pushnumber(L, heater);
        lua_call(L, 1, 0);
    }
    return 0;
}

int modulate_heater(lua_State* L) {
    luaL_checkint(L, 1);
    lua_pushlightfunction(L, libstorm_os_invoke_periodically);
    lua_pushnumber(L, SECOND_TICKS);
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, do_heater_cycle, 1);
    lua_call(L, 2, 0);
    return 0;
}

int do_heater_cycle(lua_State* L) {
    int heater = lua_tointeger(L, lua_upvalueindex(1));
    int setting = 10 * heaterSettings[heater] * MILLISECOND_TICKS;
    lua_pushlightfunction(L, check_occupancy);
    lua_call(L, 0, 1);
    if (!lua_toboolean(L, -1)) {
        setting = 0;
    }
    if (setting > 0) {
        lua_pushlightfunction(L, set_heater_state);
        lua_pushnumber(L, heater);
        lua_pushnumber(L, ON);
        lua_call(L, 2, 0);
    }
    if (setting < SECOND_TICKS) {
        lua_pushlightfunction(L, libstorm_os_invoke_later);
        lua_pushnumber(L, setting);
        lua_pushlightfunction(L, set_heater_state);
        lua_pushnumber(L, heater);
        lua_pushnumber(L, OFF);
        lua_call(L, 4, 0);
    }
    return 0;
}

/* Modulation of fans. Corresponds to the following Lua code:

local last_occupancy_state = false
storm.os.invokePeriodically(
   storm.os.SECOND,
   function ()
      local current_state = storm.n.check_occupancy()
      if current_state and not last_occupancy_state then
         for i = 1,#fans do
            fan = fans[i]
            storm.n.set_fan_state(fan, storm.n.quantize_fan(storm.n.get_fan(fan)))
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

*/

int last_occupancy_state = 0;
int check_fans(lua_State* L);

int modulate_fans(lua_State* L) {
    lua_pushlightfunction(L, libstorm_os_invoke_periodically);
    lua_pushnumber(L, SECOND_TICKS);
    lua_pushlightfunction(L, check_fans);
    lua_call(L, 2, 0);
    return 0;
}

int check_fans(lua_State* L) {
    lua_pushlightfunction(L, check_occupancy);
    lua_call(L, 0, 1);
    int current_state = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    int fan;
    
    if (current_state && !last_occupancy_state) {
        for (fan = 0; fan < NUM_FANS; fan++) {
            lua_pushlightfunction(L, set_fan_state);
            lua_pushnumber(L, fan);
            lua_pushlightfunction(L, quantize_fan);
            lua_pushnumber(L, fanSettings[fan]);
            lua_call(L, 1, 1);
            lua_call(L, 2, 0);
        }
    } else if (!current_state && last_occupancy_state) {
        for (fan = 0; fan < NUM_FANS; fan++) {
            lua_pushlightfunction(L, set_fan_state);
            lua_pushnumber(L, fan);
            lua_pushnumber(L, OFF);
            lua_call(L, 2, 0);
        }
    }
    
    last_occupancy_state = current_state;
    
    return 0;
}

/* Utilities for time synchronization */

int32_t timediff = 0;

int to_hex_str(lua_State* L) {
    uint16_t num = luaL_checkint(L, 1);
    char str[5];
    sprintf(str, "%x", num);
    lua_pushlstring(L, str, 4);
    return 1;
}

int get_time(lua_State* L) {
    if (timediff) {
        lua_pushlightfunction(L, get_time_always);
        lua_call(L, 0, 1);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int get_time_always(lua_State* L) {
    lua_pushlightfunction(L, get_kernel_secs);
    lua_call(L, 0, 1);
    int32_t time = (int32_t) lua_tointeger(L, -1) + timediff;
    lua_pop(L, 1);
    lua_pushnumber(L, time);
    return 1;
}

int get_time_diff(lua_State* L) {
    if (timediff) {
        lua_pushnumber(L, timediff);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int set_time_diff(lua_State* L) {
    int32_t timediffdiff = (int32_t) luaL_checkint(L, 1);
    lua_pushlightfunction(L, get_kernel_secs);
    lua_call(L, 0, 1);
    int32_t time = (int32_t) lua_tointeger(L, -1) + timediff + timediffdiff;
    if (time < 1400000000 || time > 1600000000) {
        printf("Time synchronization fails sanity check: %d\n", (int) time);
        lua_pushboolean(L, 0);
        return 1; // a sanity check, to make sure the time is not something crazy
    }
    if (timediff) {
        timediff = (int32_t) (timediff + ALPHA * timediffdiff);
    } else {
        timediff = timediffdiff;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int compute_time_diff(lua_State* L) {
    int64_t t0 = (int64_t) luaL_checkint(L, 1);
    int64_t t1 = (int64_t) luaL_checkint(L, 2);
    int64_t t2 = (int64_t) luaL_checkint(L, 3);
    int64_t t3 = (int64_t) luaL_checkint(L, 4);
    int64_t diff = ((t1 - t0) + (t2 - t3)) >> 1;
    lua_pushnumber(L, (int32_t) diff);
    return 1;
}

