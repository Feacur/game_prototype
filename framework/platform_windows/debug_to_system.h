#if !defined(GAME_PLATFORM_DEBUG_TO_SYSTEM)
#define GAME_PLATFORM_DEBUG_TO_SYSTEM

// interface from `debug.c` to `system.c`
// - framework initialization

#include "framework/common.h"

bool debug_to_system_init(void);
void debug_to_system_free(void);

#endif
