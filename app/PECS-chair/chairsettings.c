#include "chairsettings.h"

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
    int occ = lua_tonumber(L, occ_index); // to convert from boolean to integer
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