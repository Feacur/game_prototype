#if !defined(FRAMEWORK_PLATFORM_WINDOW)
#define FRAMEWORK_PLATFORM_WINDOW

#include "window_callbacks.h"

struct Window;

enum Window_Settings {
	WINDOW_SETTINGS_NONE = 0,
	WINDOW_SETTINGS_MINIMIZE  = (1 << 0),
	WINDOW_SETTINGS_MAXIMIZE  = (1 << 1),
	WINDOW_SETTINGS_RESIZABLE = (1 << 2),
};

struct Window_Config {
	uint32_t size_x, size_y;
	enum Window_Settings settings;
};

struct Window * platform_window_init(struct Window_Config config, struct Window_Callbacks callbacks);
void platform_window_free(struct Window * window);

bool platform_window_exists(struct Window const * window);
void platform_window_start_frame(struct Window * window);
void platform_window_draw_frame(struct Window * window);
void platform_window_end_frame(struct Window * window);

void * platform_window_get_cached_device(struct Window * window);

void platform_window_get_size(struct Window const * window, uint32_t * size_x, uint32_t * size_y);
uint32_t platform_window_get_refresh_rate(struct Window const * window, uint32_t default_value);
void platform_window_toggle_borderless_fullscreen(struct Window * window);

#endif
