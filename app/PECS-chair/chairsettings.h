int updateServer(lua_State* L);

#define CHAIRSETTINGS_SYMBOLS \
    { LSTRKEY( "update_server" ), LFUNCVAL( update_server ) },
