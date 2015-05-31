#define PA_OFFSET 0x000 // GPIO Port 0
#define PB_OFFSET 0x200 // GPIO Port 1

#define GPIO_BASE 0x400E1000
#define GPIO_ENABLE_SET 0x004
#define GPIO_ENABLE_CLEAR 0x008
#define GPIO_OUTPUT_ENABLE_SET 0x044
#define GPIO_OUTPUT_ENABLE_CLEAR 0x048
#define GPIO_OUTPUT_SET 0x054
#define GPIO_OUTPUT_CLEAR 0x058
#define GPIO_OUTPUT_TOGGLE 0x05C
#define GPIO_SCHMITT_ENABLE_SET 0x164
#define GPIO_SCHMITT_ENABLE_CLEAR 0x168
#define GPIO_PIN_VALUE 0x060
#define GPIO_PULLUP_ENABLE_SET 0x074
#define GPIO_PULLUP_ENABLE_CLEAR 0x078

#define STORM_GP12 0x00001000 // PA12 on microcontroller
#define STORM_PWM0 0x00000100 // PA08 on microcontroller
#define STORM_OCC 0x00000040 // INT1 -> PA06
#define OCC_BIT 6

#define BOTTOM_HEATER 0 // storm.n.BOTTOM_HEATER
#define BACK_HEATER 1 // storm.n.BACK_HEATER

#define BOTTOM_FAN 0 // storm.n.BOTTOM_FAN
#define BACK_FAN 1 // storm.n.BACK_FAN

#define DISABLE 0
#define ENABLE 1

#define OFF 0
#define ON 1
#define TOGGLE 2

#define FAN_CONTROLLER_ADDR 0x40
#define FAN_CONTROLLER_IODIR_ADDR 0x00
#define FAN_CONTROLLER_GPIO_ADDR 0x09

#define LOW 1
#define MEDIUM 2
#define HIGH 3
#define MAX 4

#define FAHRENHEIT 0
#define CELSIUS 1

const uint32_t heaters[] = {STORM_GP12, STORM_PWM0};

volatile uint32_t* const gpio0_enable_set = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_ENABLE_SET);
volatile uint32_t* const gpio0_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_ENABLE_CLEAR);
volatile uint32_t* const gpio0_output_enable_set = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_OUTPUT_ENABLE_SET);
volatile uint32_t* const gpio0_output_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_OUTPUT_ENABLE_CLEAR);
volatile uint32_t* const gpio0_output_set = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_OUTPUT_SET);
volatile uint32_t* const gpio0_output_clear = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_OUTPUT_CLEAR);
volatile uint32_t* const gpio0_output_toggle = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_OUTPUT_TOGGLE);

//used for occupancy detection
volatile uint32_t * const gpio0_enable_read = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET);
volatile const uint32_t* const gpio0_pin_value = (volatile const uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_PIN_VALUE);
volatile uint32_t* const gpio0_schmitt_enable_set = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_SCHMITT_ENABLE_SET);
volatile uint32_t* const gpio0_schmitt_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_SCHMITT_ENABLE_CLEAR);
volatile uint32_t* const gpio0_pullup_enable_set = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_PULLUP_ENABLE_SET);
volatile uint32_t* const gpio0_pullup_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PA_OFFSET + GPIO_PULLUP_ENABLE_CLEAR);

volatile uint32_t* const gpio1_enable_set = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_ENABLE_SET);
volatile uint32_t* const gpio1_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_ENABLE_CLEAR);
volatile uint32_t* const gpio1_output_enable_set = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_OUTPUT_ENABLE_SET);
volatile uint32_t* const gpio1_output_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_OUTPUT_ENABLE_CLEAR);
volatile uint32_t* const gpio1_output_set = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_OUTPUT_SET);
volatile uint32_t* const gpio1_output_clear = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_OUTPUT_CLEAR);
volatile uint32_t* const gpio1_output_toggle = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_OUTPUT_TOGGLE);
volatile uint32_t* const gpio1_schmitt_enable_set = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_SCHMITT_ENABLE_SET);
volatile uint32_t* const gpio1_schmitt_enable_clear = (volatile uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_SCHMITT_ENABLE_CLEAR);
volatile const uint32_t* const gpio1_pin_value = (volatile const uint32_t* const) (GPIO_BASE + PB_OFFSET + GPIO_PIN_VALUE);


int set_heater_mode(lua_State* L);
int set_heater_state(lua_State* L);

int set_fan_mode(lua_State* L);
int set_fan_state(lua_State* L);

int set_occupancy_mode(lua_State* L);
int check_occupancy(lua_State* L);

// I think this should be in libstorm.h
int libstorm_os_reset(lua_State* L);

#define CHAIRCONTROL_SYMBOLS \
    { LSTRKEY( "BOTTOM_HEATER" ), LNUMVAL( BOTTOM_HEATER ) }, \
    { LSTRKEY( "BACK_HEATER" ), LNUMVAL( BACK_HEATER ) }, \
    { LSTRKEY( "BOTTOM_FAN" ), LNUMVAL( BOTTOM_FAN ) }, \
    { LSTRKEY( "BACK_FAN" ), LNUMVAL( BACK_FAN ) }, \
    { LSTRKEY( "DISABLE" ), LNUMVAL( DISABLE ) }, \
    { LSTRKEY( "ENABLE" ), LNUMVAL( ENABLE ) }, \
    { LSTRKEY( "OFF" ), LNUMVAL( OFF ) }, \
    { LSTRKEY( "ON" ), LNUMVAL( ON ) }, \
    { LSTRKEY( "TOGGLE" ), LNUMVAL( TOGGLE ) }, \
    { LSTRKEY( "LOW" ), LNUMVAL( LOW ) }, \
    { LSTRKEY( "MEDIUM" ), LNUMVAL( MEDIUM ) }, \
    { LSTRKEY( "HIGH" ), LNUMVAL( HIGH ) }, \
    { LSTRKEY( "MAX" ), LNUMVAL( MAX ) }, \
    { LSTRKEY( "FAHRENHEIT" ), LNUMVAL( FAHRENHEIT ) }, \
    { LSTRKEY( "CELSIUS" ), LNUMVAL( CELSIUS ) }, \
    { LSTRKEY( "set_heater_mode" ), LFUNCVAL( set_heater_mode ) }, \
    { LSTRKEY( "set_heater_state" ), LFUNCVAL( set_heater_state ) }, \
    { LSTRKEY( "set_fan_mode" ), LFUNCVAL( set_fan_mode ) }, \
    { LSTRKEY( "set_fan_state" ), LFUNCVAL( set_fan_state ) }, \
    { LSTRKEY( "write_register_fan" ), LFUNCVAL( lua_write_register_fan ) }, \
    { LSTRKEY( "read_register_fan" ), LFUNCVAL( lua_read_register_fan ) }, \
    { LSTRKEY( "check_occupancy" ), LFUNCVAL( check_occupancy ) }, \
    { LSTRKEY( "set_occupancy_mode" ), LFUNCVAL( set_occupancy_mode ) }, \
    { LSTRKEY( "quantize_fan"), LFUNCVAL( quantize_fan ) }, \
    { LSTRKEY( "set_temp_mode"), LFUNCVAL( set_temp_mode ) }, \
    { LSTRKEY( "get_temp_humidity" ), LFUNCVAL( lua_get_temp_humidity ) }, \
    { LSTRKEY( "enable_reset" ), LFUNCVAL( enable_reset ) }, \
    { LSTRKEY( "gcbytes" ), LFUNCVAL( gcbytes ) },
