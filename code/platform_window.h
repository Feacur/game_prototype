#if !defined(GAME_PLATFORM_WINDOW)
#define GAME_PLATFORM_WINDOW

#include "common.h"
#include "input_keys.h"

struct Window;

struct Window * platform_window_init(void);
void platform_window_free(struct Window * window);

bool platform_window_exists(struct Window * window);
void platform_window_update(struct Window * window);

int32_t platform_window_get_vsync(struct Window * window);
void platform_window_set_vsync(struct Window * window, int32_t value);
void platform_window_display(struct Window * window);

bool platform_window_key(struct Window * window, enum Key_Code key);
bool platform_window_key_transition(struct Window * window, enum Key_Code key, bool state);

void platform_window_mouse_position_window(struct Window * window, int32_t * x, int32_t * y);
void platform_window_mouse_position_display(struct Window * window, int32_t * x, int32_t * y);
void platform_window_mouse_delta(struct Window * window, int32_t * x, int32_t * y);
bool platform_window_mouse(struct Window * window, enum Mouse_Code key);
bool platform_window_mouse_transition(struct Window * window, enum Mouse_Code key, bool state);

void platform_window_get_size(struct Window * window, uint32_t * size_x, uint32_t * size_y);
uint32_t platform_window_get_refresh_rate(struct Window * window, uint32_t default_value);

#endif
