#include "libstormarray.h"
#include "flash.h"

uint32_t cached_sp = CACHE_INVALID;
uint32_t cached_cp = CACHE_INVALID;
uint32_t cached_bo = CACHE_INVALID;

int clear_superblock_cache(lua_State* L) {
    cached_sp = CACHE_INVALID;
    cached_cp = CACHE_INVALID;
    cached_bo = CACHE_INVALID;
    return 0;
}

int call_fn(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_call(L, 1, 1);
    return 1;
}

int delay_handler(lua_State* L) {
    lua_pushlightfunction(L, libstorm_os_invoke_later);
    lua_pushnumber(L, 35 * MILLISECOND_TICKS);
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
    
    storm_array_nc_create(L, 1, ARR_TYPE_INT32);
    lua_setglobal(L, "__updatetime");
    
    storm_array_nc_create(L, 5, ARR_TYPE_INT32);
    lua_setglobal(L, "__settingsarr");
    
    lua_newtable(L);
    lua_setglobal(L, "__flashqueue");
    return 0;
}

/* Provide queue to make sure the flash reads and writes don't occur concurrently. */

int queue_empty = 1;
int front_index = 1;
int curr_index = 1;

int execute_flash_task(lua_State* L);
int finish_flash_task(lua_State* L) {
    //printf("Finished flash task\n");
    int num_args = lua_gettop(L);
    lua_pushvalue(L, lua_upvalueindex(1));
    int i;
    for (i = 1; i <= num_args; i++) {
        lua_pushvalue(L, i);
    }
    lua_call(L, num_args, 0);
    lua_pop(L, num_args);
    if (front_index == curr_index) {
        front_index = 1;
        curr_index = 1;
        queue_empty = 1;
        //printf("Resetting flash queue\n");
    } else {
        lua_pushlightfunction(L, execute_flash_task);
        lua_call(L, 0, 0);
    }
    return 0;
}

int execute_flash_task(lua_State* L) {
    lua_getglobal(L, "__flashqueue");
    int table_index = lua_gettop(L);
    lua_pushnumber(L, front_index++);
    lua_gettable(L, table_index);
    int task_index = lua_gettop(L);
    size_t task_len = lua_objlen(L, task_index);
    //printf("Executing task of length %d (tasks remaining: %d)\n", task_len, curr_index - front_index);
    int i;
    for (i = 1; i <= task_len; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, task_index);
    }
    // handling for the callback
    lua_pushcclosure(L, finish_flash_task, 1);
    
    lua_call(L, task_len - 1, 0);
    return 0;
}

int enqueue_flash_task(lua_State* L) {
    int entry_len = lua_gettop(L);
    
    lua_getglobal(L, "__flashqueue");
    int table_index = lua_gettop(L);
    
    // Create and fill in entry
    lua_createtable(L, entry_len, 0);
    int entry_index = lua_gettop(L);
    int i;
    for (i = 1; i <= entry_len; i++) {
        lua_pushnumber(L, i);
        lua_pushvalue(L, i);
        lua_settable(L, entry_index);
    }
    
    // Add entry to queue
    lua_pushnumber(L, curr_index++);
    lua_pushvalue(L, entry_index);
    lua_settable(L, table_index);
    
    //printf("Enqueued task of length %d (now %d tasks on queue)\n", lua_objlen(L, entry_index), curr_index - front_index);
    
    // Start execution if queue was empty
    if (queue_empty) {
        queue_empty = 0;
        lua_pushlightfunction(L, execute_flash_task);
        lua_call(L, 0, 0);
    }
    return 0;
}

/* Functions that write to the log. */

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

int read_sp_1(lua_State* L);
int read_sp_2(lua_State* L);
int read_sp_3(lua_State* L);
int read_sp_tail(lua_State* L);

int read_p(lua_State* L, int offset) {
    lua_pushlightfunction(L, read_sp_1);
    lua_pushnumber(L, offset);
    lua_pushvalue(L, 1);
    lua_call(L, 2, 0);
    return 0;
}

// read_sp(cb)
int read_sp(lua_State* L) {
    if (cached_sp != CACHE_INVALID) { // cache hit
        lua_pushvalue(L, 1);
        lua_pushnumber(L, cached_sp);
        lua_call(L, 1, 0);
        return 0;
    }
    return read_p(L, SP_OFFSET); // cache miss
}

int read_bo(lua_State* L) {
    if (cached_bo != CACHE_INVALID) { // cache hit
        lua_pushvalue(L, 1);
        lua_pushnumber(L,cached_bo);
        lua_call(L, 1, 0);
        return 0;
    }
    return read_p(L, BO_OFFSET); // cache miss
}

int read_cp(lua_State* L) {
    if (cached_cp != CACHE_INVALID) { // cache hit
        lua_pushvalue(L, 1);
        lua_pushnumber(L, cached_cp);
        lua_call(L, 1, 0);
        return 0;
    }
    return read_p(L, CP_OFFSET); // cache miss
}

int read_sp_1(lua_State* L) {
    int offset = luaL_checkint(L, 1);
    lua_getglobal(L, "__r1");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, (0 << PAGE_EXP) + offset);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 2); // the callback
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 1);// the offset
    lua_pushcclosure(L, read_sp_2, 3);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_sp_2(lua_State* L) {
    int offset = lua_tointeger(L, lua_upvalueindex(3));
    lua_getglobal(L, "__r2");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, (1 << PAGE_EXP) + offset);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(3)); // the offset
    lua_pushcclosure(L, read_sp_3, 4);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int read_sp_3(lua_State* L) {
    int offset = lua_tointeger(L, lua_upvalueindex(4));
    lua_getglobal(L, "__r3");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, libstorm_flash_read);
    lua_pushnumber(L, (2 << PAGE_EXP) + offset);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(3));
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, lua_upvalueindex(4)); // the offset
    lua_pushcclosure(L, read_sp_tail, 5);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int update_p(lua_State* L);

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
    int offset = lua_tointeger(L, lua_upvalueindex(5));
    int sp;
    if (sp1 == sp2 && sp2 == sp3 && ((sp1 >= FIRST_VALID_ADDR && sp1 < FIRST_INVALID_ADDR) || (offset == BO_OFFSET))) {
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
        if ((sp < FIRST_VALID_ADDR || sp >= FIRST_INVALID_ADDR) && offset != BO_OFFSET) {
            printf("Log overflowed: resetting log\n");
            sp = 3 << PAGE_EXP;
        }
        
        lua_pushnumber(L, sp);
        lua_pushvalue(L, lua_upvalueindex(5)); // the offset
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushnumber(L, sp);
        lua_pushcclosure(L, call_fn, 2);
        lua_pushcclosure(L, update_p, 3);
        
        lua_call(L, 0, 0);
    }
    return 0;
}

int write_sp_1(lua_State* L);
int write_sp_2(lua_State* L);
int write_sp_3(lua_State* L);

// Three upvalues: (1) new pointer value (2) offset (3) callback
int update_p(lua_State* L) {
    lua_pushlightfunction(L, write_sp_1);
    int i;
    for (i = 1; i <= 3; i++) {
        lua_pushvalue(L, lua_upvalueindex(i));
    }
    lua_call(L, 3, 0);
    return 0;
}

int write_p(lua_State* L, int offset) {
    lua_pushlightfunction(L, write_sp_1);
    lua_pushvalue(L, 1);
    lua_pushnumber(L, offset);
    lua_pushvalue(L, 2);
    lua_call(L, 3, 0);
    return 0;
}

int write_sp(lua_State* L) {
    cached_sp = (uint32_t) luaL_checkint(L, 1); // cache the value we're about to write in memory
    return write_p(L, SP_OFFSET);
}

int write_bo(lua_State* L) {
    cached_bo = (uint32_t) luaL_checkint(L, 1);  // cache the value we're about to write in memory
    return write_p(L, BO_OFFSET);
}

int write_cp(lua_State* L) {
    cached_cp = (uint32_t) luaL_checkint(L, 1); // cache the value we're about to write in memory
    return write_p(L, CP_OFFSET);
}

int write_sp_1(lua_State* L) {
    int new_sp = luaL_checkint(L, 1);
    int offset = luaL_checkint(L, 2);
    lua_getglobal(L, "__w1");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, arr_set);
    lua_pushvalue(L, arr_index);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, new_sp);
    lua_call(L, 3, 0);
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, (0 << PAGE_EXP) + offset);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 3); // callback
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 2); // the offset
    lua_pushcclosure(L, write_sp_2, 3);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int write_sp_2(lua_State* L) {
    int offset = lua_tointeger(L, lua_upvalueindex(3));
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, (1 << PAGE_EXP) + offset);
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(1)); // callback
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(3)); // the offset
    lua_pushcclosure(L, write_sp_3, 3);
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
    return 0;
}

int write_sp_3(lua_State* L) {
    int offset = lua_tointeger(L, lua_upvalueindex(3));
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushnumber(L, (2 << PAGE_EXP) + offset);
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, lua_upvalueindex(1)); // callback
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 3, 0);
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

// update_time(address, new_time, callback)
int update_time(lua_State* L) {
    lua_getglobal(L, "__updatetime");
    int arr_index = lua_gettop(L);
    lua_pushlightfunction(L, arr_set);
    lua_pushvalue(L, arr_index);
    lua_pushnumber(L, 1);
    lua_pushvalue(L, 2); // the new time
    lua_call(L, 3, 0);
    
    lua_pushlightfunction(L, libstorm_flash_write);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 3); // the callback
    lua_pushcclosure(L, delay_handler, 1); // the next operation may be a flash operation
    lua_call(L, 3, 0);
    
    return 0;
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

int write_log_entry_2(lua_State* L);
int write_log_entry_cb(lua_State* L);
int write_log_entry_cleanup(lua_State* L);

// write_log_entry(time_offset, backh, bottomh, backf, bottomf, temp, hum, occ, reboot, callback)
int write_log_entry(lua_State* L) {
    int i, arr_index;
    uint8_t known_time;
    uint8_t bytes[LOG_ENTRY_LEN];
    uint32_t timestamp;
    
    lua_pushlightfunction(L, get_kernel_secs);
    lua_call(L, 0, 1);
    if (lua_isnil(L, 1)) {
        timestamp = lua_tointeger(L, -1);
        known_time = 0;
    } else {
        timestamp = lua_tointeger(L, -1) + luaL_checkint(L, 1);
        known_time = 1;
    }
    lua_pop(L, 1);
    
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
    lua_pushlightfunction(L, read_cp);
    lua_pushvalue(L, arr_index);
    lua_pushvalue(L, 10); // the callback
    lua_pushcclosure(L, write_log_entry_2, 2);
    lua_call(L, 1, 0);
    return 0;
}

int write_log_entry_2(lua_State* L) {
    lua_pushlightfunction(L, read_sp);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, write_log_entry_cb, 3);
    lua_call(L, 1, 0);
    return 0;
}

uint32_t next_p(uint32_t p) {
    uint32_t new_p = p + LOG_ENTRY_LEN;
    uint32_t next_page = ((new_p >> PAGE_EXP) + 1) << PAGE_EXP;
    if ((next_page - new_p) < LOG_ENTRY_LEN) {
        new_p = next_page;
    }
    if (new_p >= FIRST_INVALID_ADDR) {
        new_p = FIRST_VALID_ADDR;
    }
    return new_p;
}

int write_log_entry_cb(lua_State* L) {
    uint32_t cp = (uint32_t) lua_tointeger(L, lua_upvalueindex(3));
    uint32_t sp = (uint32_t) lua_tointeger(L, 1);
    if (next_p(sp) == cp) {
        printf("Skipping write\n");
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushcclosure(L, delay_handler, 1);
        lua_call(L, 0, 0);
        return 0; // don't write
    }
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
    uint32_t new_sp = next_p(prev_sp);
    lua_pushlightfunction(L, write_sp);
    lua_pushnumber(L, new_sp);
    lua_pushvalue(L, lua_upvalueindex(2)); // the callback
    lua_pushcclosure(L, delay_handler, 1);
    lua_call(L, 2, 0);
    return 0;
}

int read_log_entry_addr(lua_State* L);
int read_log_entry_cb(lua_State* L);
int read_log_entry(lua_State* L) {
    int index = luaL_checkint(L, 1);
    uint32_t entries_per_page = PAGE_SIZE / LOG_ENTRY_LEN;
    uint32_t page = index / entries_per_page;
    uint32_t page_offset = index % entries_per_page;
    uint32_t flash_addr = LOG_START + (page << PAGE_EXP) + (page_offset * LOG_ENTRY_LEN);
    lua_pushlightfunction(L, read_log_entry_addr);
    lua_pushnumber(L, flash_addr);
    lua_call(L, 1, 0);
    return 0;
}

int read_log_entry_addr(lua_State* L) {
    uint32_t flash_addr = (uint32_t) luaL_checkint(L, 1);
    
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
    lua_pushnumber(L, timestamp);
    lua_pushnumber(L, occ);
    lua_pushnumber(L, backh);
    lua_pushnumber(L, bottomh);
    lua_pushnumber(L, backf);
    lua_pushnumber(L, bottomf);
    lua_pushnumber(L, temperature);
    lua_pushnumber(L, humidity);
    lua_pushnumber(L, known_time);
    lua_pushnumber(L, rebooted);
    lua_call(L, 10, 0);
    
    return 0;
}
