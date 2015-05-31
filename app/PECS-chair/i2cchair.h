#define SCL_FAN 0x00000001
#define SCL_FAN_BIT 0
#define SDA_FAN 0x00000002
#define SDA_FAN_BIT 1

#define SCL_TEMP 0x00400000
#define SCL_TEMP_BIT 22
#define SDA_TEMP 0x00200000
#define SDA_TEMP_BIT 21

#define TEMPERATURE_COMMAND 3
#define HUMIDITY_COMMAND 5

int i2c_write_byte_fan(int send_start, int send_stop, uint8_t byte);
uint8_t i2c_read_byte_fan(int nack, int send_stop);

int i2c_write_byte_temp(int send_start, int send_stop, uint8_t byte);
uint8_t i2c_read_byte_temp(int nack, int send_stop);

int lua_i2c_read_byte_fan(lua_State* L);
int lua_i2c_write_byte_fan(lua_State* L);
int lua_read_SDA_fan(lua_State* L);
int lua_read_SCL_fan(lua_State* L);
int lua_read_pins_fan(lua_State* L);
int lua_set_SDA_fan(lua_State* L);
int lua_set_SCL_fan(lua_State* L);
int lua_write_register_fan(lua_State* L);
int lua_read_register(lua_State* L);

int lua_read_bytes_temp(lua_State* L);
int lua_write_byte_temp(lua_State* L);
int lua_read_SDA_temp(lua_State* L);
int lua_read_SCL_temp(lua_State* L);
int lua_read_pins_temp(lua_State* L);
int lua_set_SDA_temp(lua_State* L);
int lua_set_SCL_temp(lua_State* L);
int lua_write_register_temp(lua_State* L);
int lua_read_register(lua_State* L);

#define I2CCHAIR_SYMBOLS \
    { LSTRKEY( "i2c_read_byte_fan" ), LFUNCVAL( lua_i2c_read_byte_fan ) }, \
    { LSTRKEY( "i2c_write_byte_fan" ), LFUNCVAL( lua_i2c_write_byte_fan ) }, \
    { LSTRKEY( "read_SDA_fan" ), LFUNCVAL( lua_read_SDA_fan ) }, \
    { LSTRKEY( "read_SCL_fan" ), LFUNCVAL( lua_read_SCL_fan ) }, \
    { LSTRKEY( "read_pins_fan" ), LFUNCVAL( lua_read_pins_fan ) }, \
    { LSTRKEY( "set_SDA_fan" ), LFUNCVAL( lua_set_SDA_fan ) }, \
    { LSTRKEY( "set_SCL_fan" ), LFUNCVAL( lua_set_SCL_fan ) }, \
    { LSTRKEY( "read_bytes_temp" ), LFUNCVAL( lua_read_bytes_temp ) }, \
    { LSTRKEY( "write_byte_temp" ), LFUNCVAL( lua_write_byte_temp ) }, \
    { LSTRKEY( "read_SDA_temp" ), LFUNCVAL( lua_read_SDA_temp ) }, \
    { LSTRKEY( "read_SCL_temp" ), LFUNCVAL( lua_read_SCL_temp ) }, \
    { LSTRKEY( "read_pins_temp" ), LFUNCVAL( lua_read_pins_temp ) }, \
    { LSTRKEY( "set_SDA_temp" ), LFUNCVAL( lua_set_SDA_temp ) }, \
    { LSTRKEY( "set_SCL_temp" ), LFUNCVAL( lua_set_SCL_temp ) },
