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

void platform_window_get_size(struct Window * window, uint32_t * size_x, uint32_t * size_y);
uint32_t platform_window_get_refresh_rate(struct Window * window, uint32_t default_value);

#endif
