#include "i2cchair.h"

int delay(int numsteps) {
    volatile int v = 0;
    int i;
    for (i = 0; i < numsteps; i++) {
        v = v + 1;
    }
    return v;
}

// I2C code is based largely on code from Wikipedia

int read_SCL_fan() {
    *gpio1_output_enable_clear = SCL_FAN;
    *gpio1_schmitt_enable_set = SCL_FAN;
    return (int) ((SCL_FAN & *gpio1_pin_value) >> SCL_FAN_BIT);
}

int read_SDA_fan() {
    *gpio1_output_enable_clear = SDA_FAN;
    *gpio1_schmitt_enable_set = SDA_FAN;
    return (int) ((SDA_FAN & *gpio1_pin_value) >> SDA_FAN_BIT);
}

void clear_SCL_fan() {
    *gpio1_schmitt_enable_clear = SCL_FAN;
    *gpio1_output_enable_set = SCL_FAN;
    *gpio1_output_clear = SCL_FAN;
}

void clear_SDA_fan() {
    *gpio1_schmitt_enable_clear = SDA_FAN;
    *gpio1_output_enable_set = SDA_FAN;
    *gpio1_output_clear = SDA_FAN;
}

int read_SCL_temp() {
    *gpio0_output_enable_clear = SCL_TEMP;
    *gpio0_schmitt_enable_set = SCL_TEMP;
    return (int) ((SCL_TEMP & *gpio0_pin_value) >> SCL_TEMP_BIT);
}

int read_SDA_temp() {
    *gpio0_output_enable_clear = SDA_TEMP;
    *gpio0_schmitt_enable_set = SDA_TEMP;
    return (int) ((SDA_TEMP & *gpio0_pin_value) >> SDA_TEMP_BIT);
}

void clear_SCL_temp() {
    *gpio0_schmitt_enable_clear = SCL_TEMP;
    *gpio0_output_enable_set = SCL_TEMP;
    *gpio0_output_clear = SCL_TEMP;
}

void clear_SDA_temp() {
    *gpio0_schmitt_enable_clear = SDA_TEMP;
    *gpio0_output_enable_set = SDA_TEMP;
    *gpio0_output_clear = SDA_TEMP;
}

void arbitration_lost() {
    printf("arbitration lost\n");
}

int started = 0; // global data
void i2c_start_cond(int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    if (started) { // if started, do a restart cond
        // set SDA to 1
        read_SDA();
        I2C_DELAY();
        read_SCL();
        // Repeated start setup time, minimum 4.7us
        I2C_DELAY();
    }
    if (read_SDA() == 0) {
        arbitration_lost();
        return;
    }
    // SCL is high, set SDA from 1 to 0.
    clear_SDA();
    I2C_DELAY();
    clear_SCL();
    started = 1;
}

void i2c_stop_cond(int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    // set SDA to 0
    clear_SDA();
    I2C_DELAY();
    read_SCL();
    // Stop bit setup time, minimum 4us
    I2C_DELAY();
    // SCL is high, set SDA from 0 to 1
    if (read_SDA() == 0) {
        arbitration_lost();
        return;
    }
    I2C_DELAY();
    started = 0;
}

// Write a bit to I2C bus
void i2c_write_bit(int bit, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    if (bit) {
        read_SDA();
    } else {
        clear_SDA();
    }
    I2C_DELAY();
    read_SCL();
    // SCL is high, now data is valid
    // If SDA is high, check that nobody else is driving SDA
    if (bit && read_SDA() == 0) {
        arbitration_lost();
        return;
    }
    I2C_DELAY();
    clear_SCL();
}

// Read a bit from I2C bus
int i2c_read_bit(int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    int bit;
    // Let the slave drive data
    read_SDA();
    I2C_DELAY();
    read_SCL();
    // SCL is high, now data is valid
    bit = read_SDA();
    I2C_DELAY();
    clear_SCL();
    return bit;
}

// Write a byte to I2C bus. Return 0 if ack by the slave.
int i2c_write_byte(int send_start, int send_stop, uint8_t byte, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    unsigned bit;
    int nack;
    if (send_start) {
        i2c_start_cond(read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    for (bit = 0; bit < 8; bit++) {
        i2c_write_bit((byte & 0x80) != 0, read_SCL, read_SDA, clear_SCL, clear_SDA);
        byte <<= 1;
    }
    nack = i2c_read_bit(read_SCL, read_SDA, clear_SCL, clear_SDA);
    if (send_stop) {
        i2c_stop_cond(read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    return nack;
}

// Read a byte from I2C bus
uint8_t i2c_read_byte(int nack, int send_stop, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    uint8_t byte = 0;
    unsigned bit;
    for (bit = 0; bit < 8; bit++) {
        byte = (byte << 1) | i2c_read_bit(read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    i2c_write_bit(nack, read_SCL, read_SDA, clear_SCL, clear_SDA);
    if (send_stop) {
        i2c_stop_cond(read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    return byte;
}

int i2c_write_byte_fan(int send_start, int send_stop, uint8_t byte) {
    return i2c_write_byte(send_start, send_stop, byte, read_SCL_fan, read_SDA_fan, clear_SCL_fan, clear_SDA_fan);
}

uint8_t i2c_read_byte_fan(int nack, int send_stop) {
    return i2c_read_byte(nack, send_stop, read_SCL_fan, read_SDA_fan, clear_SCL_fan, clear_SDA_fan);
}

void temp_start_cond(int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    read_SDA();
    clear_SCL();
    TEMP_DELAY();
    read_SCL();
    TEMP_DELAY();
    clear_SDA();
    TEMP_DELAY();
    clear_SCL();
    TEMP_DELAY();
    read_SCL();
    TEMP_DELAY();
    read_SDA();
    TEMP_DELAY();
    clear_SCL();
    TEMP_DELAY();
    clear_SDA(); // as a convention
    TEMP_DELAY();
}

void temp_write_bit(int bit, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    if (bit) {
        read_SDA();
    } else {
        clear_SDA();
    }
    TEMP_DELAY();
    read_SCL();
    TEMP_DELAY();
    clear_SCL();
    clear_SDA();
}

void temp_write_byte(uint8_t byte, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    int i;
    for (i = 0; i < 8; i++) {
        temp_write_bit((byte & 0x80) >> 7, read_SCL, read_SDA, clear_SCL, clear_SDA);
        byte = byte << 1;
    }
    TEMP_DELAY();
    read_SCL();
    TEMP_DELAY();
    read_SDA();
    TEMP_DELAY();
    clear_SCL();
    TEMP_DELAY();
}

int temp_read_bit(int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    read_SDA();
    read_SCL();
    TEMP_DELAY();
    int bit = read_SDA();
    clear_SCL();
    TEMP_DELAY();
    return bit;
}

uint8_t temp_read_byte(uint8_t nack, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    uint8_t result = 0;
    int i;
    for (i = 0; i < 8; i++) {
        result = (result << 1) | temp_read_bit(read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    temp_write_bit(nack, read_SCL, read_SDA, clear_SCL, clear_SDA);
    TEMP_DELAY();
    return result;
}

uint32_t temp_read_bytes(int numbytes, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    uint32_t result = 0;
    int i;
    for (i = 0; i < numbytes; i++) {
        result = (result << 8) | temp_read_byte((i == numbytes-1), read_SCL, read_SDA, clear_SCL, clear_SDA);
    }
    return result;
}

/* CMD must be a command where up to 4 bytes are expected in response. So, not
   all commands are supported by this function. However, it is sufficient to
   obtain temperature and humidity measurements. Use poll_temp_until_ready, where
   NUMBYTES can be at most four.
    
   Only sends the command; does not obtain measurement. Use poll_temp_until_ready
   to obtain the measurement. */
void temp_send_cmd(uint8_t cmd, int (*read_SCL)(), int (*read_SDA)(), void (*clear_SCL)(), void (*clear_SDA)()) {
    //int i;
    TEMP_DELAY();
    temp_start_cond(read_SCL, read_SDA, clear_SCL, clear_SDA);
    temp_write_byte(cmd, read_SCL, read_SDA, clear_SCL, clear_SDA);
}


/* poll_temp_until_ready(numbytes, numtries, callback)
   Repeatedly polls the sensor until the max tries is exceeded, or measurement
   (of specified number of bytes) can be obtained. NUMBYTES cannot exceed 4.
*/
int poll_temp_until_ready(lua_State* L) {
    int numbytes = luaL_checkint(L, 1);
    int numtries = luaL_checkint(L, 2);
    if (numtries == 0) {
        printf("WARNING: Could not get reading from temperature/humidity sensor (timed out)\n");
        lua_pushvalue(L, 3);
        lua_pushnumber(L, 0xFFFFFFFF);
        lua_call(L, 1, 0);
    }
    if (read_SDA_temp()) {
        lua_pushlightfunction(L, libstorm_os_invoke_later);
        lua_pushnumber(L, TEMP_POLL_PERIOD);
        lua_pushlightfunction(L, poll_temp_until_ready);
        lua_pushvalue(L, 1);
        lua_pushnumber(L, numtries - 1);
        lua_pushvalue(L, 3);
        lua_call(L, 5, 0);
    } else {
        TEMP_DELAY(); // Just to make sure we wait long enough
        uint32_t bytes = temp_read_bytes(numbytes, read_SCL_temp, read_SDA_temp, clear_SCL_temp, clear_SDA_temp);
        lua_pushvalue(L, 3);
        lua_pushnumber(L, bytes);
        lua_call(L, 1, 0);
    }
    return 0;
}

void temp_send_cmd_tempsensor(uint8_t cmd) {
    return temp_send_cmd(cmd, read_SCL_temp, read_SDA_temp, clear_SCL_temp, clear_SDA_temp);
}

// Wrapper functions for Lua

int lua_i2c_read_byte_fan(lua_State* L) {
    int nack = luaL_checkint(L, 1);
    int send_stop = luaL_checkint(L, 2);
    int result = (int) i2c_read_byte_fan(nack, send_stop);
    lua_pushnumber(L, result);
    return 1;
}

int lua_i2c_write_byte_fan(lua_State* L) {
    int send_start = luaL_checkint(L, 1);
    int send_stop = luaL_checkint(L, 2);
    uint8_t byte = (uint8_t) luaL_checkint(L, 3);
    int nack = i2c_write_byte_fan(send_start, send_stop, byte);
    lua_pushnumber(L, nack);
    return 1;
}

int lua_read_bytes_temp(lua_State* L) {
    int numbytes = luaL_checkint(L, 1);
    uint32_t result = temp_read_bytes(numbytes, read_SCL_temp, read_SDA_temp, clear_SCL_temp, clear_SDA_temp);
    lua_pushnumber(L, (int) result);
    return 1;
}

int lua_write_byte_temp(lua_State* L) {
    int send_start = luaL_checkint(L, 1);
    uint8_t byte = (uint8_t) luaL_checkint(L, 2);
    if (send_start) {
        temp_start_cond(read_SCL_temp, read_SDA_temp, clear_SCL_temp, clear_SDA_temp);
    }
    temp_write_byte(byte, read_SCL_temp, read_SDA_temp, clear_SCL_temp, clear_SDA_temp);
    return 0;
}

int lua_read_SDA_fan(lua_State* L) {
    int sda = read_SDA_fan();
    lua_pushnumber(L, sda);
    return 1;
}

int lua_read_SCL_fan(lua_State* L) {
    int scl = read_SCL_fan();
    lua_pushnumber(L, scl);
    return 1;
}

int lua_read_SDA_temp(lua_State* L) {
    int sda = read_SDA_temp();
    lua_pushnumber(L, sda);
    return 1;
}

int lua_read_SCL_temp(lua_State* L) {
    int scl = read_SCL_temp();
    lua_pushnumber(L, scl);
    return 1;
}

int lua_set_SDA_fan(lua_State* L) {
    *gpio1_output_enable_set = SDA_FAN;
    if (luaL_checkint(L, 1)) {
        *gpio1_output_set = SDA_FAN;
    } else {
        *gpio1_output_clear = SDA_FAN;
    }
    return 1;
}

int lua_set_SCL_fan(lua_State* L) {
    *gpio1_output_enable_set = SCL_FAN;
    if (luaL_checkint(L, 1)) {
        *gpio1_output_set = SCL_FAN;
    } else {
        *gpio1_output_clear = SCL_FAN;
    }
    return 1;
}

int lua_set_SDA_temp(lua_State* L) {
    *gpio0_output_enable_set = SDA_TEMP;
    if (luaL_checkint(L, 1)) {
        *gpio0_output_set = SDA_TEMP;
    } else {
        *gpio0_output_clear = SDA_TEMP;
    }
    return 1;
}

int lua_set_SCL_temp(lua_State* L) {
    *gpio0_output_enable_set = SCL_TEMP;
    if (luaL_checkint(L, 1)) {
        *gpio0_output_set = SCL_TEMP;
    } else {
        *gpio0_output_clear = SCL_TEMP;
    }
    return 1;
}

int lua_read_pins_fan(lua_State* L) {
    lua_pushnumber(L, *gpio1_pin_value);
    return 1;
}

int lua_read_pins_temp(lua_State* L) {
    lua_pushnumber(L, *gpio0_pin_value);
    return 1;
}

