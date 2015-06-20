int actuation_handler(lua_State* L);
int bl_handler(lua_State* L);
int to_hex_str(lua_State* L);

#define RECEIVER_SYMBOLS \
    { LSTRKEY("bytes_to_u32"), LFUNCVAL(bytes_to_u32) }, \
    { LSTRKEY( "actuation_handler" ), LFUNCVAL( actuation_handler ) }, \
    { LSTRKEY( "bl_handler" ), LFUNCVAL( bl_handler ) },
