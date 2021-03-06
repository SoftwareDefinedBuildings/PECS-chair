#include <interface.h>
#include "blchair.h"

extern lua_State* _cb_L;
int received_bl = 0; // has anything been received via bluetooth since this was last set to 0?
volatile int32_t bl_receive_flag;
char bl_receive_buffer[20];
const char ping[4] = { 0, 0, 0, 0 };

// Inspired by syscall types in libstorm.c
int32_t __attribute__((naked)) k_syscall_ex_ri32(uint32_t id) {
    __syscall_body(ABI_ID_SYSCALL_EX);
}
int32_t __attribute__((naked)) k_syscall_ex_ri32_ccptr_u32(uint32_t id, const char* string, uint32_t len) {
    __syscall_body(ABI_ID_SYSCALL_EX);
}
int32_t __attribute__((naked)) k_syscall_ex_ri32_cptr_u32_vi32ptr_vptr(uint32_t id, char* buffer, uint32_t len, volatile int32_t* flag, void* callback) {
    __syscall_body(ABI_ID_SYSCALL_EX);
}

// Define syscalls for using the PECS Bluetooth
#define bl_PECS_init_syscall() k_syscall_ex_ri32(0x5700)
#define bl_PECS_send_syscall(data, len) k_syscall_ex_ri32_ccptr_u32(0x5702, (data), (len))
#define bl_PECS_receive_syscall(buffer, len, flag, cb) k_syscall_ex_ri32_cptr_u32_vi32ptr_vptr(0x5703, (buffer), (len), (flag), (cb))
#define bl_PECS_clearbuf_syscall() k_syscall_ex_ri32(0x5704)

int do_nothing(lua_State* L) {
    return 0;
}

int blank_count = 0;
// Don't call this like a Lua function!
int reset_hmsoft_if_inactive(lua_State* L) {
    if (received_bl) {
        received_bl = 0;
        blank_count = 0;
    } else if (++blank_count == 5) {
        blank_count = 0;
        printf("Resetting HMSoft (was inactive for 20 seconds)\n");
        lua_pushlightfunction(L, reset_hmsoft);
        lua_call(L, 0, 0);
        return 0;
    }
    return 1;
}

int ping_bl(lua_State* L) {
    if (reset_hmsoft_if_inactive(L)) {
        bl_PECS_send_syscall(ping, 4);
    }
    return 0;
}

int periodically_ping_bl(lua_State* L) {
    lua_pushlightfunction(L, libstorm_os_invoke_periodically);
    lua_pushnumber(L, SECOND_TICKS << 2);
    lua_pushlightfunction(L, ping_bl);
    lua_call(L, 2, 0);
    return 0;
}

void print_byte(uint8_t byte) {
    printf("%d\n", byte);
}

int bl_PECS_init(lua_State* L) {
    int32_t result = bl_PECS_init_syscall();
    lua_pushnumber(L, result);
    return 1;
}

int bl_PECS_send(lua_State* L) {
    size_t length;
    const char* data = luaL_checklstring(L, 1, &length);
    lua_pushnumber(L, length);
    bl_PECS_send_syscall(data, (int) length);
    return 1;
}

void bl_PECS_receive_cb_handler();

int bl_PECS_receive_cb(lua_State* L) {
    int toRead = luaL_checkint(L, 1);
    if (toRead > 20) {
        return 0;
    }
    
    lua_pushvalue(L, 2); // the callback
    lua_setglobal(L, "__bl_cb");
    
    bl_receive_flag = 0;
    bl_PECS_receive_syscall(bl_receive_buffer, toRead, &bl_receive_flag, (void*) bl_PECS_receive_cb_handler);
    
    return 0;
}

void bl_PECS_receive_cb_handler() {
    int rv;
    const char* msg;
    received_bl = 1; // we got something via bluetooth
    lua_getglobal(_cb_L, "__bl_cb"); // the callback
    lua_pushlstring(_cb_L, bl_receive_buffer, bl_receive_flag);
    rv = lua_pcall(_cb_L, 1, 0, 0);
    if (rv) {
        printf("[ERROR] could not run bl_PECS callback (%d)\n", rv);
        msg = lua_tostring(_cb_L, -1);
        printf("[ERROR] msg: %s\n", msg);
    }
}

int bl_PECS_clear_recv_buf(lua_State* L) {
    bl_PECS_clearbuf_syscall();
    return 0;
}

int interpret_string(lua_State* L) {
    size_t length;
    int i;
    const char* string = luaL_checklstring(L, 1, &length);
    for (i = 0; i < length; i++) {
        lua_pushnumber(L, string[i]);
    }
    return (int) length;
}

char strbuf[11];
int pack_string(lua_State* L) {
    strbuf[0] = luaL_checkint(L, 1); // back heater
    strbuf[1] = luaL_checkint(L, 2); // bottom heater
    strbuf[2] = luaL_checkint(L, 3); // back fan
    strbuf[3] = luaL_checkint(L, 4); // bottom fan
    strbuf[4] = luaL_checkint(L, 5); // occupancy
    uint16_t temp = (uint16_t) luaL_checkint(L, 6);
    uint16_t humidity = (uint16_t) luaL_checkint(L, 7);
    strbuf[5] = temp >> 8;
    strbuf[6] = temp & 0xFF; // big endian temperature reading
    strbuf[7] = humidity >> 8;
    strbuf[8] = humidity & 0xFF; // big endian humidity reading
    uint16_t macaddr = (uint16_t) luaL_checkint(L, 8);
    strbuf[9] = macaddr >> 8;
    strbuf[10] = macaddr & 0xFF; // big endian node id
    lua_pushlstring(L, strbuf, 11);
    return 1;
}

void write_big_endian_16(uint16_t data, uint8_t* buffer) {
    buffer[0] = data >> 8;
    buffer[1] = data & 0x00FF;
}

void write_big_endian_32(uint32_t data, uint8_t* buffer) {
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t) ((data & 0x00FF0000) >> 16);
    buffer[2] = (uint8_t) ((data & 0x0000FF00) >> 8);
    buffer[3] = (uint8_t) (data & 0x000000FF);
}

char lstrbuf[19];
int pack_large_string(lua_State* L) {
    lstrbuf[0] = (char) luaL_checkint(L, 1); // back heater
    lstrbuf[1] = (char) luaL_checkint(L, 2); // bottom heater
    lstrbuf[2] = (char) luaL_checkint(L, 3); // back fan
    lstrbuf[3] = (char) luaL_checkint(L, 4); // bottom fan
    lstrbuf[4] = (char) luaL_checkint(L, 5); // occupancy
    
    uint16_t temp = (uint16_t) luaL_checkint(L, 6);
    write_big_endian_16(temp, (uint8_t*) (lstrbuf + 5));
    
    uint16_t humidity = (uint16_t) luaL_checkint(L, 7);
    write_big_endian_16(humidity, (uint8_t*) (lstrbuf + 7));
    
    uint16_t macaddr = (uint16_t) luaL_checkint(L, 8);
    write_big_endian_16(macaddr, (uint8_t*) (lstrbuf + 9));
    
    uint32_t timestamp = (uint32_t) luaL_checkint(L, 9);
    write_big_endian_32(timestamp, (uint8_t*) (lstrbuf + 11));
    
    uint32_t ack_id = (uint32_t) luaL_checkint(L, 10);
    write_big_endian_32(ack_id, (uint8_t*) (lstrbuf + 15));
    
    lua_pushlstring(L, lstrbuf, 19);
    return 1;
}
