int init_autosender(lua_State* L);

#define AUTOSENDER_SYMBOLS \
    { LSTRKEY("autosender_init"), LFUNCVAL(init_autosender) }, \
    { LSTRKEY("autosender_register_time_sync"), LFUNCVAL(register_time_synchronization) },
