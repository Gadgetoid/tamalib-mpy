#include <stdio.h>
#include <stdarg.h>

#include "py/dynruntime.h"
#include "py/misc.h"
#include "lib/tamalib.h"

#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#include "nanoprintf.h"

uint32_t matrix_buffer[LCD_HEIGHT] = {0};
uint8_t icon_buffer[8] = {0};

//timestamp_t ts;
//timestamp_t sleep_until;
bool tamalib_is_late;

void *memset(void *str, int c, size_t n) {
    unsigned char *ptr = str;
    while(n--) {
        *ptr = c;
        ptr++;
    }
    return str;
}

static void * hal_malloc(u32_t size)
{
	return NULL; // m_tracked_calloc(size, sizeof(uint8_t));
}

static void hal_free(void *ptr)
{
    //m_tracked_free(ptr);
}

bool halt = false;
static void hal_halt(void)
{
    if(halt) return;
    halt = true;
    mp_printf(&mp_plat_print, "Halt!?\n");
}

static bool_t hal_is_log_enabled(log_level_t level)
{
	return 0; //level < 3 ? 1 : 0;
}

static void hal_log(log_level_t level, char *fmt, ...)
{   
    return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    //sprintf(buf, fmt, args);
    npf_vsnprintf(buf, sizeof(buf), fmt, args);
    mp_printf(&mp_plat_print, "%d: %s\n", level, buf);
    va_end(args);
}

static timestamp_t hal_get_timestamp(void)
{
    // Call out to a MicroPython function here
	return (timestamp_t) (*(uint32_t *)0x40054028) >> 1;
}

static void hal_sleep_until(timestamp_t ts)
{
    // Call out to a MicroPython function here?
	if (((int32_t)(ts - hal_get_timestamp())) > 0) {
		tamalib_is_late = false;
	}
    //mp_printf(&mp_plat_print, "Sleep until %lu (%lu) %d\n", ts, hal_get_timestamp(), tamalib_is_late);

    //sleep_until = ts;
}

static void hal_update_screen(void)
{
    // We're not using mainloop so this never gets called?
}

static void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    //mp_printf(&mp_plat_print, "Set pixel %d:%d=%d\n", x, y, val);
    // TODO: Make sure our bit ordering makes sense
    // The display is 32bits wide so our buffer is just uint32 x 16
    // DO we *really* need to be this frugal?
    matrix_buffer[y] &= ~(1 << x);
    if (val) {
	    matrix_buffer[y] |= 1 << x;
    }
}

static void hal_set_lcd_icon(u8_t icon, bool_t val)
{
    //mp_printf(&mp_plat_print, "Set icon %d %d\n", icon, val);
	// Straight call out to MicroPython here?
    icon_buffer[icon] = val;
}

static void hal_set_frequency(u32_t freq)
{
	// Straight call out to MicroPython here?
    //mp_printf(&mp_plat_print, "Set freq %llu\n", freq);
}

static void hal_play_frequency(bool_t en)
{
	// Straight call out to MicroPython here?
    //mp_printf(&mp_plat_print, "Play freq %d\n", en);
}

static int hal_handler(void)
{
	return 0;
}

const hal_t hal = {
	.malloc = &hal_malloc,
	.free = &hal_free,
	.halt = &hal_halt,
	.is_log_enabled = &hal_is_log_enabled,
	.log = &hal_log,
	.sleep_until = &hal_sleep_until,
	.get_timestamp = &hal_get_timestamp,
	.update_screen = &hal_update_screen,
	.set_lcd_matrix = &hal_set_lcd_matrix,
	.set_lcd_icon = &hal_set_lcd_icon,
	.set_frequency = &hal_set_frequency,
	.play_frequency = &hal_play_frequency,
	.handler = &hal_handler,
};

// A simple function that adds its 2 arguments (must be integers)
static mp_obj_t init(mp_obj_t bin_in) {
    //ts = 0;
    tamalib_register_hal(&hal);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(bin_in, &bufinfo, MP_BUFFER_READ);

    bool fail = tamalib_init((u12_t *)bufinfo.buf, NULL, 1000000); // my_breakpoints can be NULL, 1000000 means that timestamps will be expressed in us

    if(fail) {
        mp_printf(&mp_plat_print, "failed to init...\n");
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(init_obj, init);

static mp_obj_t step() {
   // for(unsigned i = 0; i < 1000; i++) {
    tamalib_is_late = true;
    while(tamalib_is_late) {
        //ts = mp_obj_get_int(ts_in);
        //if(ts < sleep_until) return mp_const_none;
        tamalib_step();
    }
    //}
    return halt ? mp_const_false : mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(step_obj, step);

static mp_obj_t display() {
    mp_obj_tuple_t *display = MP_OBJ_TO_PTR(mp_obj_new_tuple(16, NULL));
    for (int y = 0; y < 16; y++) {
        display->items[y] = mp_obj_new_int(matrix_buffer[y]);
    }
    return MP_OBJ_FROM_PTR(display);
}
static MP_DEFINE_CONST_FUN_OBJ_0(display_obj, display);

static mp_obj_t icons() {
    mp_obj_tuple_t *icons = MP_OBJ_TO_PTR(mp_obj_new_tuple(8, NULL));
    for (int i = 0; i < 8; i++) {
        icons->items[i] = icon_buffer[i] ? mp_const_true : mp_const_false;
    }
    return MP_OBJ_FROM_PTR(icons);
}
static MP_DEFINE_CONST_FUN_OBJ_0(icons_obj, icons);



static mp_obj_t button(mp_obj_t button_in, mp_obj_t state_in) {
    int button = mp_obj_get_int(button_in);
    bool state = state_in == mp_const_true;
    tamalib_set_button(button, state);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(button_obj, button);

// This is the entry point and is called when the module is imported
mp_obj_t mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args) {
    // This must be first, it sets up the globals dict and other things
    MP_DYNRUNTIME_INIT_ENTRY

    // Messages can be printed as usual
    mp_printf(&mp_plat_print, "initialising module self=%p\n", self);

    // Make the functions available in the module's namespace
    mp_store_global(MP_QSTR_init, MP_OBJ_FROM_PTR(&init_obj));
    mp_store_global(MP_QSTR_step, MP_OBJ_FROM_PTR(&step_obj));
    mp_store_global(MP_QSTR_display, MP_OBJ_FROM_PTR(&display_obj));
    mp_store_global(MP_QSTR_icons, MP_OBJ_FROM_PTR(&icons_obj));
    mp_store_global(MP_QSTR_button, MP_OBJ_FROM_PTR(&button_obj));

    // Add some constants to the module's namespace
    mp_store_global(MP_QSTR_BUTTON_LEFT, MP_OBJ_NEW_SMALL_INT(BTN_LEFT));
    mp_store_global(MP_QSTR_BUTTON_MIDDLE, MP_OBJ_NEW_SMALL_INT(BTN_MIDDLE));
    mp_store_global(MP_QSTR_BUTTON_RIGHT, MP_OBJ_NEW_SMALL_INT(BTN_RIGHT));

    mp_store_global(MP_QSTR_WIDTH, MP_OBJ_NEW_SMALL_INT(32));
    mp_store_global(MP_QSTR_HEIGHT, MP_OBJ_NEW_SMALL_INT(16));
    //mp_store_global(MP_QSTR_MSG, MP_OBJ_NEW_QSTR(MP_QSTR_HELLO_MICROPYTHON));

    // This must be last, it restores the globals dict
    MP_DYNRUNTIME_INIT_EXIT
}
