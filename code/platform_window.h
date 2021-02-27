#if !defined(GAME_PLATFORM_WINDOW)
#define GAME_PLATFORM_WINDOW

#include "common.h"
#include "input_keys.h"

struct Window;

struct Window * platform_window_init(void);
void platform_window_free(struct Window * window);

bool platform_window_exists(struct Window * window);
void platform_window_update(struct Window * window);

bool platform_window_key(struct Window * window, enum Key_Code key);
bool platform_window_key_transition(struct Window * window, enum Key_Code key, bool state);

bool platform_window_mouse(struct Window * window, enum Mouse_Code key);
bool platform_window_mouse_transition(struct Window * window, enum Mouse_Code key, bool state);

uint16_t platform_window_get_refresh_rate(struct Window * window, uint16_t default_value);

#endif
