#define PAGE_SIZE 256
#define PAGE_EXP 8

#define LOG_ENTRY_LEN 14
#define LOG_START 768

#define SP_OFFSET 0
#define BO_OFFSET 8
#define CP_OFFSET 16

#define FIRST_VALID_ADDR 768
#define FIRST_INVALID_ADDR 7340032

#define CACHE_INVALID 0xFFFFFFFF

int read_sp(lua_State* L);
int write_sp(lua_State* L);

// I think these should be in libstorm.h
int libstorm_flash_read(lua_State* L);
int libstorm_flash_write(lua_State* L);

#define FLASH_SYMBOLS \
    { LSTRKEY("get_kernel_secs"), LFUNCVAL(get_kernel_secs) }, \
    { LSTRKEY("flash_clear_cache"), LFUNCVAL(clear_superblock_cache) }, \
    { LSTRKEY("flash_init"), LFUNCVAL(flash_init) }, \
    { LSTRKEY("flash_read_sp"), LFUNCVAL(read_sp) }, \
    { LSTRKEY("flash_write_sp"), LFUNCVAL(write_sp) }, \
    { LSTRKEY("flash_read_bo"), LFUNCVAL(read_bo) }, \
    { LSTRKEY("flash_write_bo"), LFUNCVAL(write_bo) }, \
    { LSTRKEY("flash_read_cp"), LFUNCVAL(read_cp) }, \
    { LSTRKEY("flash_write_cp"), LFUNCVAL(write_cp) }, \
    { LSTRKEY("flash_update_time"), LFUNCVAL(update_time) }, \
    { LSTRKEY("flash_get_log_size"), LFUNCVAL(get_log_size) }, \
    { LSTRKEY("flash_write_log"), LFUNCVAL(write_log_entry) }, \
    { LSTRKEY("flash_read_log"), LFUNCVAL(read_log_entry) }, \
    { LSTRKEY("flash_save_settings"), LFUNCVAL(save_settings) }, \
    { LSTRKEY("flash_get_saved_settings"), LFUNCVAL(get_saved_settings) }, \
    { LSTRKEY("enqueue_flash_task"), LFUNCVAL(enqueue_flash_task) },
