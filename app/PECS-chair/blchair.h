int bl_PECS_write(lua_State* L);
int bl_PECS_send(lua_State* L);
int bl_PECS_receive_cb(lua_State* L);
int interpret_string(lua_State* L);
int pack_string(lua_State* L);

#define BLCHAIR_SYMBOLS \
    { LSTRKEY( "bl_PECS_init" ), LFUNCVAL( bl_PECS_init) }, \
    { LSTRKEY( "bl_PECS_send" ), LFUNCVAL( bl_PECS_send ) }, \
    { LSTRKEY( "bl_PECS_receive_cb" ), LFUNCVAL( bl_PECS_receive_cb ) }, \
    { LSTRKEY( "bl_PECS_clear_recv_buf" ), LFUNCVAL( bl_PECS_clear_recv_buf ) }, \
    { LSTRKEY( "interpret_string" ), LFUNCVAL( interpret_string ) }, \
    { LSTRKEY( "pack_string" ), LFUNCVAL( pack_string ) },
