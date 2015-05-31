#define ALPHA 0.2

int actuation_handler(lua_State* L);
int bl_handler(lua_State* L);
int to_hex_str(lua_State* L);
int get_time_always(lua_State* L);
int get_time(lua_State* L);
int get_time_diff(lua_State* L);
int set_time_diff(lua_State* L);
int compute_time_diff(lua_State* L);

#define RECEIVER_SYMBOLS \
    { LSTRKEY( "actuation_handler" ), LFUNCVAL( actuation_handler ) }, \
    { LSTRKEY( "bl_handler" ), LFUNCVAL( bl_handler ) }, \
    { LSTRKEY( "to_hex_str" ), LFUNCVAL( to_hex_str ) }, \
    { LSTRKEY( "get_time_always" ), LFUNCVAL( get_time_always ) }, \
    { LSTRKEY( "get_time" ), LFUNCVAL( get_time ) }, \
    { LSTRKEY( "get_time_diff" ), LFUNCVAL( get_time_diff ) }, \
    { LSTRKEY( "set_time_diff" ), LFUNCVAL( set_time_diff ) }, \
    { LSTRKEY( "compute_time_diff" ), LFUNCVAL( compute_time_diff ) },
