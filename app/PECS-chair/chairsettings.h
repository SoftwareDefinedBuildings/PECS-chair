#ifndef __CHAIRSETTINGS_H__
#define __CHAIRSETTINGS_H__

#define ALPHA 0.2

int chairsettings_init(lua_State* L);
int set_heater(lua_State* L);
int set_fan(lua_State* L);
int get_heater(lua_State* L);
int get_fan(lua_State* L);
int update_server(lua_State* L);
int modulate_heaters(lua_State* L);
int modulate_fans(lua_State* L);
int get_time_always(lua_State* L);
int get_time(lua_State* L);
int get_time_diff(lua_State* L);
int set_time_diff(lua_State* L);
int compute_time_diff(lua_State* L);

#define CHAIRSETTINGS_SYMBOLS \
    { LSTRKEY( "chairsettings_init"), LFUNCVAL( chairsettings_init ) }, \
    { LSTRKEY( "set_heater" ), LFUNCVAL( set_heater ) }, \
    { LSTRKEY( "set_fan" ), LFUNCVAL( set_fan ) }, \
    { LSTRKEY( "get_heater" ), LFUNCVAL( get_heater ) }, \
    { LSTRKEY( "get_fan" ), LFUNCVAL( get_fan ) }, \
    { LSTRKEY( "update_server" ), LFUNCVAL( update_server ) }, \
    { LSTRKEY( "modulate_heaters" ), LFUNCVAL( modulate_heaters ) }, \
    { LSTRKEY( "modulate_fans" ), LFUNCVAL( modulate_fans ) }, \
    { LSTRKEY( "to_hex_str" ), LFUNCVAL( to_hex_str ) }, \
    { LSTRKEY( "get_time_always" ), LFUNCVAL( get_time_always ) }, \
    { LSTRKEY( "get_time" ), LFUNCVAL( get_time ) }, \
    { LSTRKEY( "get_time_diff" ), LFUNCVAL( get_time_diff ) }, \
    { LSTRKEY( "set_time_diff" ), LFUNCVAL( set_time_diff ) }, \
    { LSTRKEY( "compute_time_diff" ), LFUNCVAL( compute_time_diff ) },

#endif