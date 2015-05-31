#include <interface.h>
#include "blchair.h"

extern lua_State* _cb_L;
volatile int32_t bl_receive_flag;
char bl_receive_buffer[20];

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

char strbuf[9];
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
    lua_pushlstring(L, strbuf, 9);
    return 1;
}
