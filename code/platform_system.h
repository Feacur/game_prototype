#if !defined(GAME_PLATFORM_SYSTEM)
#define GAME_PLATFORM_SYSTEM

#include "common.h"

void platform_system_init(void);
void platform_system_free(void);

bool platform_window_is_running(void);
void platform_system_update(void);

void platform_system_sleep(uint32_t millis);

#endif
