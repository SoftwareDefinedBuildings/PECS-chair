#include "libstormarray.h"
#include "flash.h"

int call_fn(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_call(L, 1, 1);
    return 1;
}

int delay_handler(lua_State* L) {
    lua_pushlightfunction(L, libstorm_os_invoke_later);
    lua_pushnumber(L, 30 * MILLISECOND_TICKS);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_call(L, 2, 0);
    return 0;
}

int flash_init(lua_State* L) {
    storm_array_nc_create(L, 1, ARR_TYPE_INT32);
    lua_setglobal(L, "__r1");
    storm_array_nc_create(L, 1, ARR_TYPE_INT32);
    lua_setglobal(L, "__r2");
    storm_array_nc_create(L, 1, ARR_TYPE_INT32);
    lua_setglobal(L, "__r3");
    storm_array_nc_create(L, 1, ARR_TYPE_INT32);
    lua_setglobal(L, "__w1");
    storm_array_nc_create(L, LOG_ENTRY_LEN, ARR_TYPE_UINT8);
    lua_setglobal(L, "__writearr");
    storm_array_nc_create(L, LOG_ENTRY_LEN, ARR_TYPE_UINT8);
    lua_setglobal(L, "__readarr");
    
    storm_array_nc_create(L, 5, ARR_TYPE_INT32);
    lua_setglobal(L, "__settingsarr");
    return 0;
}

int save_settings(lua_State* L) {
    lua_getglobal(L, "__settingsarr");
    int arr_index = lua_gettop(L);
    int i;
    for (i = 1; i <= 5; i++) {
        lua_pushlightfunction(L, arr_set);
        lua_pushvalue(L, arr_index);
        lua_pushnumber(L, i);
        lua_pushvalue(L, i);
        lua_call(L, 3, 0);
    }
    
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, 100);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 6); // the callback
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int get_saved_settings_cb(lua_State* L);
int get_saved_settings(lua_State* L) {
    lua_getglobal(L, "__settingsarr");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, 100);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 1); // the callback
    lua_pushcclosure(L, get_saved_settings_cb, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int get_saved_settings_cb(lua_State* L) {
    int arr_index = lua_upvalueindex(1);
    lua_pushvalue(L, lua_upvalueindex(2)); // the callback
    int i, val;
    int valid = 1;
    for (i = 1; i <= 5; i++) {
        lua_pushlightfunction(L, arr_get);
        lua_pushvalue(L, arr_index);
        lua_pushnumber(L, i);
        lua_call(L, 2, 1);
        val = lua_tonumber(L, -1);
        if (val < 0 || val > 100) { // check if something got corrupted
            valid = 0;
            break;
        }
    }
    if (!valid) {
        lua_pop(L, i);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    lua_call(L, 5, 0);
    return 0;
}

int read_sp_2(lua_State* L);
int read_sp_3(lua_State* L);
int read_sp_tail(lua_State* L);
// read_sp(cb)
int read_sp(lua_State* L) {
    lua_getglobal(L, "__r1");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, 0 << PAGE_EXP);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 1); // the callback
    lua_pushvalue(L, arr_index);
    lua_pushcclosure(L, read_sp_2, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_sp_2(lua_State* L) {
    lua_getglobal(L, "__r2");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, 1 << PAGE_EXP);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, arr_index);
    lua_pushcclosure(L, read_sp_3, 3);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_sp_3(lua_State* L) {
    lua_getglobal(L, "__r3");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, 2 << PAGE_EXP);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(3));
    lua_pushvalue(L, arr_index);
    lua_pushcclosure(L, read_sp_tail, 4);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_sp_tail(lua_State* L) {
    int i;
    for (i = 2; i < 5; i++) {
        lua_pushlightfunction(L, arr_get);
        lua_pushvalue(L, lua_upvalueindex(i));
        lua_pushnumber(L, 1);
        lua_call(L, 2, 1);
    }
    int sp1 = lua_tointeger(L, 1);
    int sp2 = lua_tointeger(L, 2);
    int sp3 = lua_tointeger(L, 3);
    int sp;
    if (sp1 == sp2 && sp2 == sp3 && sp1 >= 0 && sp1 < 0x600000) {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushnumber(L, sp1);
        lua_call(L, 1, 0);
    } else {
        printf("Corrupt log pointer: pointers %d %d %d conflict. Repairing...\n", sp1, sp2, sp3);
        if (sp1 == sp2) {
            sp = sp1;
        } else if (sp2 == sp3) {
            sp = sp2;
        } else if (sp1 == sp3) {
            sp = sp3;
        } else {
            sp = sp3;
        }
        if (sp < 0 || sp >= 0x600000) {
            printf("Log overflowed: resetting log\n");
            sp = 3 << PAGE_EXP;
        }
        lua_pushlightfunction(L, libstorm_os_invoke_later);
        lua_pushnumber(L, 70 * MILLISECOND_TICKS);
        lua_pushlightfunction(L, write_sp);
        lua_pushnumber(L, sp);
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushnumber(L, sp);
        lua_pushcclosure(L, call_fn, 2);
        lua_call(L, 4, 0);
    }
    return 0;
}

int write_sp_2(lua_State* L);
int write_sp_3(lua_State* L);
int write_sp(lua_State* L) {
    int new_sp = luaL_checkint(L, 1);
    lua_getglobal(L, "__w1");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, arr_set);
    lua_pushvalue(L, arr_index);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, new_sp);
    lua_call(L, 3, 0);
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, 0 << PAGE_EXP);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 2); // callback
    lua_pushvalue(L, arr_index);
    lua_pushcclosure(L, write_sp_2, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int write_sp_2(lua_State* L) {
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, 1 << PAGE_EXP);
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(1)); // callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushcclosure(L, write_sp_3, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int write_sp_3(lua_State* L) {
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, 2 << PAGE_EXP);
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(1)); // callback
    lua_call(L, 3, 0);
    return 0;
}

int reset_log(lua_State* L) {
    lua_pushlightfunction(L, write_sp);
    lua_pushnumber(L, 3 << PAGE_EXP);
    lua_pushvalue(L, 1); // callback
    lua_call(L, 2, 0);
    return 0;
}

int get_log_size_tail(lua_State* L);
int get_log_size(lua_State* L) {
    lua_pushlightfunction(L, read_sp);
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, get_log_size_tail, 1);
    lua_call(L, 1, 0);
    return 0;
}

int get_log_size_tail(lua_State* L) {
    uint32_t sp = (uint32_t) luaL_checkint(L, 1);
    sp -= LOG_START;
    uint32_t page = sp >> PAGE_EXP;
    uint32_t page_offset = sp - (page << PAGE_EXP);
    int entries_per_page = PAGE_SIZE / LOG_ENTRY_LEN;
    int index = page * entries_per_page + page_offset / LOG_ENTRY_LEN;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushnumber(L, index);
    lua_call(L, 1, 0);
    return 0;
}

int bytes_to_timestamp(lua_State* L) {
    uint32_t byte1 = (uint32_t) luaL_checkint(L, 1);
    uint32_t byte2 = (uint32_t) luaL_checkint(L, 2);
    uint32_t byte3 = (uint32_t) luaL_checkint(L, 3);
    uint32_t byte4 = (uint32_t) luaL_checkint(L, 4);
    uint32_t timestamp = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
    lua_pushnumber(L, timestamp);
    return 1;
}

int get_kernel_secs(lua_State* L) {
    lua_pushlightfunction(L, libstorm_os_now);
    lua_pushnumber(L, 2);
    lua_call(L, 1, 1);
    uint32_t timeshift1 = (uint32_t) lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_pushlightfunction(L, libstorm_os_now);
    lua_pushnumber(L, 3);
    lua_call(L, 1, 1);
    uint32_t timeshift2 = (uint32_t) lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // we can ignore timer_getnow(). It is 16 bits and milliseconds are 375 ticks, so we'll be off by at most 175 ms, which is OK.
    uint64_t uppertime = (((uint64_t) timeshift2) << 32) | ((uint64_t) timeshift1);
    uint32_t seconds = (uint32_t) (8192 * uppertime / 46875.0); // convert to seconds
    lua_pushnumber(L, seconds);
    return 1;
}

/** FORMAT OF A LOG ENTRY
    Each log entry has the following format:
    1. Timestamp (32 bits)
    2. Back Heater Setting (8 bits)
    3. Bottom Heater Setting (8 bits)
    4. Back Fan Setting (8 bits)
    5. Bottom Fan Setting (8 bits)
    6. Temperature Reading (16 bits)
    7. Relative Humidity Reading (16 bits)
    8. Occupancy (1 bit)
    9. Reboot Indicator (1 bit)
    10. Known Time Indicator (1 bit)
    11. Extra Space (13 bits)
    Total of 112 bits (14 bytes) per entry
**/

int write_log_entry_cb(lua_State* L);
int write_log_entry_cleanup(lua_State* L);

// write_log_entry(time_offset, backh, bottomh, backf, bottomf, temp, hum, occ, reboot, callback)
int write_log_entry(lua_State* L) {
    int i, arr_index;
    uint8_t known_time;
    uint8_t bytes[LOG_ENTRY_LEN];
    uint32_t timestamp;
    
    if (lua_isnil(L, 1)) {
        timestamp = 0;
        known_time = 0;
    } else {
        lua_pushlightfunction(L, get_kernel_secs);
        lua_call(L, 0, 1);
        timestamp = lua_tointeger(L, -1) + luaL_checkint(L, 1);
        lua_pop(L, 1);
        known_time = 1;
    }
    uint8_t backh = (uint8_t) luaL_checkint(L, 2);
    uint8_t bottomh = (uint8_t) luaL_checkint(L, 3);
    uint8_t backf = (uint8_t) luaL_checkint(L, 4);
    uint8_t bottomf = (uint8_t) luaL_checkint(L, 5);
    uint16_t temperature = (uint16_t) luaL_checkint(L, 6);
    uint16_t humidity = (uint16_t) luaL_checkint(L, 7);
    luaL_checktype(L, 9, LUA_TBOOLEAN);
    uint8_t secondlastbyte = (((uint8_t) luaL_checkint(L, 8)) << 7) | ((uint8_t) lua_toboolean(L, 9) << 6) | (known_time << 5); // for now, the time is always known
    
    *((uint32_t*) bytes) = timestamp;
    bytes[4] = backh;
    bytes[5] = bottomh;
    bytes[6] = backf;
    bytes[7] = bottomf;
    *((uint16_t*) (bytes + 8)) = temperature;
    *((uint16_t*) (bytes + 10)) = humidity;
    bytes[12] = secondlastbyte;
    bytes[13] = 0; // extra space
    
    // create array to write
    lua_getglobal(L, "__writearr");
    arr_index = lua_gettop(L);
    for (i = 0; i < LOG_ENTRY_LEN; i++) {
        lua_pushlightfunction(L, arr_set);
        lua_pushvalue(L, arr_index);
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, bytes[i]);
        lua_call(L, 3, 0);
    }
    
    // read the stack pointer, write, and then update it
    lua_pushlightfunction(L, read_sp);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 10); // the callback
    lua_pushcclosure(L, write_log_entry_cb, 2);
    lua_call(L, 1, 0);
    return 0;
}

int write_log_entry_cb(lua_State* L) {
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushcclosure(L, write_log_entry_cleanup, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int write_log_entry_cleanup(lua_State* L) {
    uint32_t prev_sp = lua_tonumber(L, lua_upvalueindex(1));
    uint32_t new_sp = prev_sp + LOG_ENTRY_LEN;
    uint32_t next_page = ((new_sp >> PAGE_EXP) + 1) << PAGE_EXP;
    if (next_page - new_sp < LOG_ENTRY_LEN) {
        new_sp = next_page;
    }
    lua_pushlightfunction(L, write_sp);
    lua_pushnumber(L, new_sp);
    lua_pushvalue(L, lua_upvalueindex(2)); // the callback
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 2, 0);
    return 0;
}

int read_log_entry_cb(lua_State* L);
int read_log_entry(lua_State* L) {
    int index = luaL_checkint(L, 1);
    uint32_t entries_per_page = PAGE_SIZE / LOG_ENTRY_LEN;
    uint32_t page = index / entries_per_page;
    uint32_t page_offset = index % entries_per_page;
    uint32_t flash_addr = LOG_START + (page << PAGE_EXP) + (page_offset * LOG_ENTRY_LEN);
    
    lua_getglobal(L, "__readarr");
    int arr_index = lua_gettop(L);
    
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, flash_addr);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 2); // the callback
    lua_pushcclosure(L, read_log_entry_cb, 2);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_log_entry_cb(lua_State* L) {
    int i;
    int arr_index = lua_upvalueindex(1);
    uint8_t bytes[LOG_ENTRY_LEN];
    for (i = 0; i < LOG_ENTRY_LEN; i++) {
        lua_pushlightfunction(L, arr_get);
        lua_pushvalue(L, arr_index);
        lua_pushnumber(L, i + 1);
        lua_call(L, 2, 1);
        bytes[i] = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    
    uint32_t timestamp = *((uint32_t*) bytes);
    uint8_t backh = bytes[4];
    uint8_t bottomh = bytes[5];
    uint8_t backf = bytes[6];
    uint8_t bottomf = bytes[7];
    uint16_t temperature = *((uint16_t*) (bytes + 8));
    uint16_t humidity = *((uint16_t*) (bytes + 10));
    uint8_t secondlastbyte = bytes[12];
    uint8_t occ = (secondlastbyte >> 7);
    uint8_t rebooted = (secondlastbyte >> 6) & 0x1;
    uint8_t known_time = (secondlastbyte >> 5) & 0x1;
    
    lua_pushvalue(L, lua_upvalueindex(2)); // the callback
    if (known_time) {
        lua_pushnumber(L, timestamp);
    } else {
        lua_pushnil(L);
    }
    lua_pushnumber(L, backh);
    lua_pushnumber(L, bottomh);
    lua_pushnumber(L, backf);
    lua_pushnumber(L, bottomf);
    lua_pushnumber(L, temperature);
    lua_pushnumber(L, humidity);
    lua_pushnumber(L, occ);
    lua_pushnumber(L, rebooted);
    lua_call(L, 9, 0);
    
    return 0;
}
