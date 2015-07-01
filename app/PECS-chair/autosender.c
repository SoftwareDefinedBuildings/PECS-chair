#include "autosender.h"

extern int32_t timediff; // if this is 0, then time hasn't been synchronized

uint32_t expected_ack = 0xFFFFFFFF;

int session_bp = 0; // the bp for the reboot corresponding to this session
int32_t curr_diff = 0; // the offset we are currently using for relative timestamps

int in_current_session = 0;

int32_t registered_synchronization = 0;

/*
sp = "store pointer": the memory address where the next log entry will be stored
cp = "current pointer": the memory address of the next log entry to be processed
bp = "base pointer": the memory address of the reboot log entry corresponding to the last processed entry
bo = "base offset": the time offset contained in the log entry that the bp refers to
*/

int ack_handler(lua_State* L);
int init_autosender_1(lua_State* L);
int begin_send_iter(lua_State* L);

int empty(lua_State* L); // implemented in rnq.c

int update_logged_time_offset(lua_State* L) {
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, update_time);
    lua_pushnumber(L, session_bp);
    lua_pushnumber(L, registered_synchronization);
    lua_pushlightfunction(L, empty);
    lua_call(L, 4, 0);
    return 0;
}

int register_time_synchronization(lua_State* L) {
    if (!registered_synchronization) {
        registered_synchronization = timediff;
        if (session_bp) {
            lua_pushlightfunction(L, update_logged_time_offset);
            lua_call(L, 0, 0);
        }
    }
    return 0;
}

// Flash should be initialized first!
// Takes as an argument the bp of the reboot corresponding to this session
int init_autosender(lua_State* L) {
    printf("Initializing autosender...\n");
    
    session_bp = luaL_checkint(L, 1);
    if (registered_synchronization) { // synchronization was already registered! Need to process it now
        lua_pushlightfunction(L, update_logged_time_offset);
        lua_call(L, 0, 0); // by the time this task is dequeued, reboot would have already been logged. So it's OK to updated the logged time offset
    }
    
    lua_createtable(L, 0, 1);
    lua_pushstring(L, "rv");
    lua_pushstring(L, "ok");
    lua_settable(L, -3);
    lua_setglobal(L, "__default_resp");
    
    // Create new RNQ Server to listen for acknowledgement
    lua_pushlightfunction(L, rnqserver_new);
    lua_pushnil(L);
    lua_pushnumber(L, 20000);
    lua_pushlightfunction(L, ack_handler);
    lua_call(L, 3, 1);
    lua_setglobal(L, "__ack_server");
    
    // Create new RNQ Client to send requests
    lua_pushlightfunction(L, rnqclient_new);
    lua_pushnil(L);
    lua_pushnumber(L, 20001);
    lua_call(L, 2, 1);
    lua_setglobal(L, "__data_client");
    
    printf("Created RNQ Client...\n");
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, read_bo);
    lua_pushlightfunction(L, init_autosender_1);
    lua_call(L, 2, 0);
    return 0;
}

int init_autosender_1(lua_State* L) {
    printf("init_autosender_1\n");
    curr_diff = lua_tointeger(L, 1);
    lua_pushlightfunction(L, begin_send_iter);
    lua_call(L, 0, 0);
    return 0;
}

int register_ack(lua_State* L);

int ack_handler(lua_State* L) {
    lua_pushstring(L, "ack");
    lua_gettable(L, 1);
    if (!lua_isnil(L, -1)) {
        lua_pushlightfunction(L, register_ack);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);
    }
    lua_getglobal(L, "__default_resp");
    return 1;
}

int finished_send_message(lua_State* L) {
    printf("Finished setup cycle\n");
    return 0;
}

int start_process_loop(lua_State* L);

int begin_send_iter(lua_State* L) {
    printf("Beginning iteration...\n");
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, start_process_loop);
    lua_pushlightfunction(L, finished_send_message);
    lua_call(L, 2, 0);
    return 0;
}

int process_next_entry(lua_State* L);
int finish_processing_reboot_entry(lua_State* L);
int take_entry_action(lua_State* L);

int read_pointers(lua_State* L);
int start_process_loop(lua_State* L) {
    printf("Starting process loop...\n");
    lua_pushvalue(L, 1); // the callback
    lua_setglobal(L, "__autosend_cb");

    lua_pushlightfunction(L, read_pointers);
    lua_pushlightfunction(L, take_entry_action);
    lua_call(L, 1, 0);
    return 0;
}

// Callbacks to read sp and cp
// read_pointers(cb)
// calls cb with three arguments, namely sp, cp, and bo

int read_pointers_1(lua_State* L);
int read_pointers_2(lua_State* L);
int read_pointers_3(lua_State* L);

int read_pointers(lua_State* L) {
    lua_pushlightfunction(L, read_sp);
    lua_pushvalue(L, 1); // the callback
    lua_pushcclosure(L, read_pointers_1, 1);
    lua_call(L, 1, 0);
    return 0;
}

int read_pointers_1(lua_State* L) {
    lua_pushlightfunction(L, read_cp);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, 1); // sp
    lua_pushcclosure(L, read_pointers_2, 2);
    lua_call(L, 1, 0);
    return 0;
}

int read_pointers_2(lua_State* L) {
    lua_pushlightfunction(L, read_bo);
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2)); // sp
    lua_pushvalue(L, 1); // cp
    lua_pushcclosure(L, read_pointers_3, 3);
    lua_call(L, 1, 0);
    return 0;
}

int read_pointers_3(lua_State* L) {
    printf("Finished reading pointers\n");
    lua_pushvalue(L, lua_upvalueindex(1)); // the callback
    lua_pushvalue(L, lua_upvalueindex(2)); // sp
    lua_pushvalue(L, lua_upvalueindex(3)); // cp
    lua_pushvalue(L, 1); // bo
    lua_call(L, 3, 0);
    return 0;
}

// Given the three pointers, decide how to proceed
int dispatch_log_entry(lua_State* L);
int take_entry_action(lua_State* L) {
    int sp = luaL_checkint(L, 1);
    int cp = luaL_checkint(L, 2);
    int bo = luaL_checkint(L, 3);
    if (cp == session_bp) {
        in_current_session = 1;
    }
    printf("sp = %d, cp = %d, bo = %d\n", sp, cp, bo);
    curr_diff = (int32_t) bo;
    if (cp == sp || (in_current_session && !timediff)) {
        // We're publishing realtime data now OR we're entering the data for this session, but haven't synchronized time yet
        // So, we'll wait a bit and try again
        printf("Wait a bit and try again\n");
        lua_pushlightfunction(L, libstorm_os_invoke_later);
        lua_pushnumber(L, 10 * SECOND_TICKS);
        lua_pushlightfunction(L, begin_send_iter);
        lua_call(L, 2, 0);
        
        lua_getglobal(L, "__autosend_cb");
        lua_call(L, 0, 0); // so next flash event can fire
    } else {
        printf("Reading log entry\n");
        lua_pushlightfunction(L, read_log_entry_addr);
        lua_pushnumber(L, cp);
        lua_pushnumber(L, cp);
        lua_pushcclosure(L, dispatch_log_entry, 1);
        lua_call(L, 2, 0);
    }
    return 0;
}

// Given the contents of a log entry, decide what action to take
// Arguments: the same as those passed to the callback of read_log_entry
// Gets cp as an upvalue
int send_log_entry(lua_State* L);
int dispatch_log_entry(lua_State* L) {
    int rebooted = lua_tointeger(L, 10);
    int i;
    printf("Dispatching on entry...\n");
    if (rebooted) {
        printf("Handling reboot...\n");
        curr_diff = (uint32_t) lua_tointeger(L, 1);
        lua_pushlightfunction(L, write_bo);
        lua_pushnumber(L, curr_diff);
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushcclosure(L, finish_processing_reboot_entry, 1);
        lua_call(L, 2, 0);
    } else {
        printf("Handling regular entry...\n");
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushcclosure(L, send_log_entry, 1);
        for (i = 1; i <= 9; i++) {
            lua_pushvalue(L, i);
        }
        lua_call(L, 9, 0);
    }
    
    return 0;
}

int try_send(lua_State* L);

/* This is like the updateSMAP function from a previous version of chairsettings.lua, with a few important differences.
   First, it doesn't poll the current settings, but instead relies on them being passed as arguments.
   Second, it doesn't write to flash. It just sends the data.
   Third, it waits for an acknowledgement that the data has been received. */
// Previous cp passed as upvalue
int send_log_entry(lua_State* L) {
    // Get the node id (macaddr)
    lua_pushlightfunction(L, libstorm_os_getnodeid);
    lua_call(L, 0, 1);
    int nodeid_index = lua_gettop(L);
    
    // Create table to send via 15.4
    lua_createtable(L, 10, 0);
    int table_index = lua_gettop(L);
    
    int i;
    int32_t true_time = 0; // I'm initializing it to 0 to suppress a compiler warning
    for (i = 1; i <= 10; i++) {
        lua_pushnumber(L, i);
        switch (i) {
        case 1:
            lua_pushvalue(L, nodeid_index);
            break;
        case 2: // occ
        case 3: // backh
        case 4: // bottomh
        case 5: // backf
        case 6: // bottomf
        case 7: // temp
        case 8: // humidity
            lua_pushvalue(L, i);
            break;
        case 9: // timestamp
            // check if known; if not, obtain the offset
            if (lua_tointeger(L, 9)) { // if we already have an exact timestamp...
                true_time = lua_tointeger(L, 1);
            } else if (curr_diff) { // if we have a relative timestamp but know the time offset from the reboot entry...
                true_time = lua_tointeger(L, 1) + curr_diff;
            } else { // we have no choice but to skip this entry
                printf("Timestamp cannot be found. Skipping entry...\n");
                lua_pushvalue(L, lua_upvalueindex(1));
                lua_pushcclosure(L, finish_processing_reboot_entry, 1);
                lua_call(L, 0, 0);
                return 0;
            }
            lua_pushnumber(L, true_time);
            break;
        case 10: // message id
            lua_pushvalue(L, lua_upvalueindex(1));
            break;
        }
        lua_settable(L, table_index);
    }
    
    lua_setglobal(L, "__rnqMessage");
    
    printf("RNQ message done\n");
    
    // Create string to send via bluetooth
    lua_pushlightfunction(L, pack_large_string);
    for (i = 1; i <= 10; i++) {
        switch (i) {
        case 1:
        case 2:
        case 3:
        case 4:
            lua_pushvalue(L, i + 2);
            break;
        case 5:
            lua_pushvalue(L, 2);
            break;
        case 6:
        case 7:
            lua_pushvalue(L, i + 1);
            break;
        case 8:
            lua_pushvalue(L, nodeid_index);
            break;
        case 9:
            lua_pushnumber(L, true_time);
            break;
        case 10:
            lua_pushvalue(L, lua_upvalueindex(1));
            break;
        }
    }
    lua_call(L, 10, 1);
    
    // string is at the top of the stack
    lua_setglobal(L, "__blString");
    
    printf("blString done\n");
    
    expected_ack = (uint32_t) lua_tointeger(L, lua_upvalueindex(1));
    
    // Try to send data, and then check for an acknowledgement
    lua_pushlightfunction(L, try_send);
    lua_call(L, 0, 0);
    
    lua_getglobal(L, "__autosend_cb");
    lua_call(L, 0, 0); // so next flash event can fire
    
    return 0;
}

int try_send(lua_State* L) {
    printf("Trying to send...\n");
    
    // try to send via bluetooth
    lua_pushlightfunction(L, bl_PECS_send);
    lua_getglobal(L, "__blString");
    lua_call(L, 1, 0);
    
    printf("Sent bluetooth\n");
    
    // try to send via 15.4
    lua_pushlightfunction(L, rnqclient_sendMessage);
    lua_getglobal(L, "__data_client"); // an RNQ Client
    lua_getglobal(L, "__rnqMessage");
    lua_getglobal(L, "data_ip");
    lua_pushnumber(L, 38003);
    lua_pushnumber(L, 15);
    lua_pushnumber(L, SECOND_TICKS);
    lua_pushnil(L);
    lua_pushlightfunction(L, try_send); // if it times out, try again
    lua_call(L, 8, 0);
    
    printf("Function returned\n");
    // For testing purposes only. REMOVE FOR PRODUCTION.
    /*lua_pushlightfunction(L, libstorm_os_invoke_later);
    lua_pushnumber(L, 500 * MILLISECOND_TICKS);
    lua_pushlightfunction(L, register_ack);
    lua_pushnumber(L, expected_ack);
    lua_call(L, 3, 0);*/
    
    return 0;
}

int register_ack(lua_State* L) {
    uint32_t received_ack = (uint32_t) luaL_checkint(L, 1);
    if (received_ack == expected_ack) {
        printf("Got expected ack. registering...\n");
        expected_ack = 0xFFFFFFFF;
        lua_pushlightfunction(L, rnqclient_empty);
        lua_getglobal(L, "__data_client");
        lua_call(L, 1, 0);
        
        lua_pushlightfunction(L, rnqclient_cancelMessage);
        lua_getglobal(L, "__data_client");
        lua_call(L, 1, 0);
        
        lua_pushnumber(L, received_ack);
        lua_pushcclosure(L, process_next_entry, 1);
        lua_call(L, 0, 0);
    }
    return 0;
}

int finish_processing_reboot_entry(lua_State* L) {
    printf("Skipping send step...\n");
    lua_getglobal(L, "__autosend_cb");
    lua_call(L, 0, 0); // so next flash event can fire
    
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushcclosure(L, process_next_entry, 1);
    lua_call(L, 0, 0);
    
    return 0;
}

// Previous cp passed as upvalue
int process_next_entry(lua_State* L) {
    printf("Proceeding to next entry\n");
    
    uint32_t cp = (uint32_t) lua_tointeger(L, lua_upvalueindex(1));
    uint32_t new_cp = next_p(cp);
    
    lua_pushlightfunction(L, enqueue_flash_task);
    lua_pushlightfunction(L, write_cp);
    lua_pushnumber(L, new_cp);
    lua_pushlightfunction(L, begin_send_iter);
    lua_call(L, 3, 0);
    
    return 0;
}
