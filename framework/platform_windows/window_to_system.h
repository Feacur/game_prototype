#if !defined(GAME_PLATFORM_WINDOW_TO_SYSTEM)
#define GAME_PLATFORM_WINDOW_TO_SYSTEM

// interface from `window.c` to `system.c`
// - framework initialization

#include "framework/common.h"

bool window_to_system_init(void);
void window_to_system_free(void);

#endif
