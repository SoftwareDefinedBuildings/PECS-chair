int rnqclient_new(lua_State* L);
int rnqclient_sendMessage(lua_State* L);
int rnqclient_cancelMessage(lua_State* L);
int rnqclient_empty(lua_State* L);
int rnqclient_close(lua_State* L);

int rnqserver_new(lua_State* L);
int rnqserver_close(lua_State* L);

static const LUA_REG_TYPE rnqclient_meta_map[] = {
    { LSTRKEY("new"), LFUNCVAL(rnqclient_new) },
    { LSTRKEY("sendMessage"), LFUNCVAL(rnqclient_sendMessage) },
    { LSTRKEY("cancelMessage"), LFUNCVAL(rnqclient_cancelMessage) },
    { LSTRKEY("empty"), LFUNCVAL(rnqclient_empty) },
    { LSTRKEY("close"), LFUNCVAL(rnqclient_close) },
    { LSTRKEY("__index"), LROVAL(rnqclient_meta_map) }, 
    { LNILKEY, LNILVAL },
};

static const LUA_REG_TYPE rnqserver_meta_map[] = {
    { LSTRKEY("new"), LFUNCVAL(rnqserver_new) },
    { LSTRKEY("close"), LFUNCVAL(rnqserver_close) },
    { LSTRKEY("__index"), LROVAL(rnqserver_meta_map) },
    { LNILKEY, LNILVAL },
};

#define RNQ_SYMBOLS \
    { LSTRKEY( "RNQClient" ), LROVAL(rnqclient_meta_map) }, \
    { LSTRKEY( "RNQServer" ), LROVAL(rnqserver_meta_map) },
