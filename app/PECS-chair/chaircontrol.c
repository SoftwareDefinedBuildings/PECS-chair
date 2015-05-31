#include <math.h>

#include "chaircontrol.h"
#include "i2cchair.c"

uint8_t fan_gpio;

/* storm.n.check_occupancy()
    return bool if someone is in the seat */
int check_occupancy(lua_State* L) {
    uint32_t occ_pin = STORM_OCC;
    uint32_t reading = (occ_pin & *gpio0_pin_value) >> OCC_BIT;
    lua_pushboolean(L, reading^1); // reading is 0 when shorted (someone is sitting in chair)
    return 1;
}

int quantize_fan(lua_State* L) {
    int setting = luaL_checkint(L, 1);
    if (setting == 0) {
        lua_pushnumber(L, OFF);
    } else if (setting < 25) {
        lua_pushnumber(L, LOW);
    } else if (setting < 50) {
        lua_pushnumber(L, MEDIUM);
    } else if (setting < 75) {
        lua_pushnumber(L, HIGH);
    } else {
        lua_pushnumber(L, MAX);
    }
    return 1;
}

/* storm.n.set_occupancy_mode(mode)
   heater is in {storm.n.BOTTOM_HEATER, storm.n.BACK_HEATER}
   mode is in {storm.n.ENABLE, storm.n.DISABLE} */
int set_occupancy_mode(lua_State* L) {
   uint32_t occ_pin = STORM_OCC;
   int mode = luaL_checkint(L, 1);

    switch(mode) {
    case DISABLE:
            *gpio0_enable_clear = occ_pin;
            *gpio0_pullup_enable_clear = occ_pin;
            *gpio0_schmitt_enable_clear = occ_pin;
            break;
    case ENABLE:
            *gpio0_enable_set = occ_pin;
            *gpio0_pullup_enable_set = occ_pin;
            *gpio0_schmitt_enable_set = occ_pin;
            break;
    }

    return 0;
}

/* storm.n.set_heater_mode(heater, mode)
   heater is in {storm.n.BOTTOM_HEATER, storm.n.BACK_HEATER}
   mode is in {storm.n.ENABLE, storm.n.DISABLE} */
int set_heater_mode(lua_State* L) {
    uint32_t heater_pin = heaters[luaL_checkint(L, 1)];
    int mode = luaL_checkint(L, 2);

    switch(mode) {
    case DISABLE:
        *gpio0_output_enable_clear = heater_pin;
        *gpio0_enable_clear= heater_pin;
        break;
    case ENABLE:
        *gpio0_enable_set = heater_pin;
        *gpio0_output_enable_set = heater_pin;
        break;
    }

    return 0;
}

/* storm.n.set_heater_state(heater, state)
   heater is in {storm.n.BOTTOM_HEATER, storm.n.BACK_HEATER}
   state is in {storm.n.ON, storm.n.OFF, storm.n.TOGGLE}
   Heater must be enabled first. */
int set_heater_state(lua_State* L) {
    uint32_t heater_pin = heaters[luaL_checkint(L, 1)];
    int state = luaL_checkint(L, 2);

    switch(state) {
    case OFF:
        *gpio0_output_clear = heater_pin;
        break;
    case ON:
        *gpio0_output_set = heater_pin;
        break;
    case TOGGLE:
        *gpio0_output_toggle = heater_pin;
        break;
    }
    return 0;
}

/** Writes BYTE to the fan controller at the specifed REGISTER_ADDR. */
int write_register_fan(uint8_t register_addr, uint8_t byte) {
    return i2c_write_byte_fan(1, 0, FAN_CONTROLLER_ADDR)
        || i2c_write_byte_fan(0, 0, register_addr)
        || i2c_write_byte_fan(0, 1, byte);
}

/** Reads BYTE from the fan controller at the specified REGISTER_ADDR.
    Returns -1 upon error. */
int16_t read_register_fan(uint8_t register_addr) {
    if (i2c_write_byte_fan(1, 0, FAN_CONTROLLER_ADDR | 0b00000001)
        || i2c_write_byte_fan(0, 0, register_addr)) {
        return -1;
    }
    return (int16_t) i2c_read_byte_fan(1, 1);
}

/* storm.n.set_fan_mode(mode)
   mode is in {storm.n.ENABLE, storm.n.DISABLE} */
int set_fan_mode(lua_State* L) {
    int result1, result2;
    switch(luaL_checkint(L, 1)) {
    case ENABLE:
        *gpio1_enable_set = SDA_FAN;
        *gpio1_enable_set = SCL_FAN;
        result1 = write_register_fan(FAN_CONTROLLER_IODIR_ADDR, 0b10001000);
        result2 = write_register_fan(FAN_CONTROLLER_GPIO_ADDR, 0b00000000);
        fan_gpio = 0b00000000;
        break;
    case DISABLE:
        *gpio1_enable_clear = SDA_FAN;
        *gpio1_enable_clear = SCL_FAN;
        result1 = write_register_fan(FAN_CONTROLLER_IODIR_ADDR, 0b11111111);
        result2 = write_register_fan(FAN_CONTROLLER_GPIO_ADDR, 0b00000000);
        fan_gpio = 0b00000000;
        break;
    default:
        return 0;
    }
    lua_pushnumber(L, result1 && result2);
    return 1;
}

/* storm.n.set_fan_state(fan, state)
   fan is in {storm.n.BOTTOM_FAN, storm.n.BACK_FAN}
   state is in {storm.n.OFF, storm.n.LOW, storm.n.MEDIUM, storm.n.HIGH, storm.n.MAX}
   Fans must be enabled first. */
int set_fan_state(lua_State* L) {
    int fan = luaL_checkint(L, 1);
    int state = luaL_checkint(L, 2);
    uint8_t mask;
    switch(fan) {
    case BACK_FAN:
        mask = 0b11111000;
        break;
    case BOTTOM_FAN:
        mask = 0b10001111;
        break;
    default:
      return 0;
    }
    fan_gpio = fan_gpio & mask;
    mask = ~mask;
    switch(state) {
    case LOW:
        fan_gpio = fan_gpio | (0b00100010 & mask);
        break;
    case MEDIUM:
        fan_gpio = fan_gpio | (0b00010001 & mask);
        break;
    case HIGH:
        fan_gpio = fan_gpio | (0b00110011 & mask);
        break;
    case MAX:
        fan_gpio = fan_gpio | (0b01000100 & mask);
        break;
    }
    int result = write_register_fan(FAN_CONTROLLER_GPIO_ADDR, fan_gpio);
    lua_pushnumber(L, result);
    return 1;
}

int lua_write_register_fan(lua_State* L) {
    int reg = luaL_checkint(L, 1);
    int byte = luaL_checkint(L, 2);
    int ret = write_register_fan((uint8_t) reg, (uint8_t) byte);
    lua_pushnumber(L, ret);
    return 1;
}

int lua_read_register_fan(lua_State* L) {
    int reg = luaL_checkint(L, 1);
    int16_t ret = read_register_fan((uint8_t) reg);
    lua_pushnumber(L, ret);
    return 1;
}

/* storm.n.set_temp_mode(mode)
   mode is in {storm.n.ENABLE, storm.n.DISABLE} */
int set_temp_mode(lua_State* L) {
    switch(luaL_checkint(L, 1)) {
    case ENABLE:
        *gpio0_enable_set = SDA_TEMP;
        *gpio0_enable_set = SCL_TEMP;
        break;
    case DISABLE:
        *gpio0_enable_clear = SDA_TEMP;
        *gpio0_enable_clear = SCL_TEMP;
        break;
    default:
        return 0;
    }
    return 0;
}

/* storm.n.get_temp_humidity(unit)
   unit is in {storm.n.FAHRENHEIT, storm.n.CELSIUS} */
int lua_get_temp_humidity(lua_State* L) {
    int unit = luaL_checkint(L, 1);
    uint32_t reading = temp_get_reading_tempsensor(TEMPERATURE_COMMAND, 3);
    double rawtemp = (double) (reading >> 8); // remove the 8-bit checksum
    double temperature = NAN;
    double ctemp;
    switch (unit) {
    case FAHRENHEIT:
        temperature = -39.4 + 0.018 * rawtemp; // intentionally fall through
    case CELSIUS:
        ctemp = -39.65 + 0.01 * rawtemp;
        switch (unit) {
            case CELSIUS:
                temperature = ctemp;
                break;
        }
        break;
    default:
        ctemp = NAN;
        break;
    }
    reading = temp_get_reading_tempsensor(HUMIDITY_COMMAND, 3);
    double rawhumidity = (double) (reading >> 8); // remove the 8-bit checksum
    double linhumidity = -2.0468 + 0.0367 * rawhumidity - (1.5955e-6) * rawhumidity * rawhumidity;
    double humidity = (ctemp - 25) * (0.01 + 0.00008 * rawhumidity) + linhumidity;
    int fixedPtTemperature = (int) (temperature * 100);
    int fixedPtHumidity = (int) (humidity * 100); // * 100 fixed point
    lua_pushnumber(L, fixedPtTemperature);
    lua_pushnumber(L, fixedPtHumidity);
    return 2;
}

int reset_counter(lua_State* L) {
    lua_pushnumber(L, 0);
    lua_setglobal(L, "__reset_counter");
    return 0;
}

#define HMSOFT_RESET_PIN 6  // storm.io.D6
#define TP_IRQ 7  // storm.io.D7

int enable_hmsoft_and_reset(lua_State* L) {
    // storm.io.set(1, HMSOFT_RESET_PIN)
    lua_pushlightfunction(L, libstorm_io_set);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, HMSOFT_RESET_PIN);
    lua_call(L, 2, 0);
    // storm.os.reset()
    lua_pushlightfunction(L, libstorm_os_reset);
    lua_call(L, 0, 0);
    return 0;
}

int handle_tap(lua_State* L) {
    printf("Tap received\n");
    // count = count + 1
    lua_getglobal(L, "__reset_counter");
    int count = lua_tointeger(L, -1);
    count++;
    lua_pushnumber(L, count);
    lua_setglobal(L, "__reset_counter");
    // if count == 4
    if (count == 4) {
        printf("Resetting because of tap sequence\n");
        //  storm.io.set_mode(storm.io.OUTPUT, HMSOFT_RESET_PIN)
        lua_pushlightfunction(L, libstorm_io_set_mode);
        lua_pushnumber(L, 0);  // storm.io.OUTPUT
        lua_pushnumber(L, HMSOFT_RESET_PIN);
        lua_call(L, 2, 0);
        // storm.io.set(0, HMSOFT_RESET)
        lua_pushlightfunction(L, libstorm_io_set);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, HMSOFT_RESET_PIN);
        lua_call(L, 2, 0);
        lua_pushlightfunction(L, libstorm_os_invoke_later);
        lua_pushnumber(L, 2000 * MILLISECOND_TICKS);
        lua_pushlightfunction(L, enable_hmsoft_and_reset);
        lua_call(L, 2, 0);
        return 0;
    }
    // storm.os.watch_single(storm.io.FALLING, TP_IRQ, handle_change)
    lua_pushlightfunction(L, libstorm_io_watch_single);
    lua_pushnumber(L, 2);  // storm.io.FALLING = 2
    lua_pushnumber(L, TP_IRQ);
    lua_pushlightfunction(L, handle_tap);
    lua_call(L, 3, 0);
    return 0;
}

int enable_reset(lua_State* L) {
    printf("Enabling reset\n");
    //  storm.io.set_mode(storm.io.INPUT, TP_IRQ)
    lua_pushlightfunction(L, libstorm_io_set_mode);
    lua_pushnumber(L, 1);  // storm.io.INPUT
    lua_pushnumber(L, TP_IRQ);
    lua_call(L, 2, 0);
    // __reset_counter = 0
    lua_pushnumber(L, 0);
    lua_setglobal(L, "__reset_counter");
    // storm.os.invokePeriodically(storm.os.SECOND, reset_counter)
    lua_pushlightfunction(L, libstorm_os_invoke_periodically);
    lua_pushnumber(L, 1000 * MILLISECOND_TICKS);
    lua_pushlightfunction(L, reset_counter);
    lua_call(L, 2, 0);
    // storm.os.watch_single(storm.io.FALLING, TP_IRQ, handle_change)
    lua_pushlightfunction(L, libstorm_io_watch_single);
    lua_pushnumber(L, 2);  // storm.io.FALLING = 2
    lua_pushnumber(L, TP_IRQ);
    lua_pushlightfunction(L, handle_tap);
    lua_call(L, 3, 0);
    return 0;
}

int gcbytes(lua_State* L) {
    int kbytes = lua_gc(L, LUA_GCCOUNT, 0);
    int bytes = lua_gc(L, LUA_GCCOUNTB, 0);
    lua_pushnumber(L, (kbytes << 10) | bytes);
    return 1;
}
