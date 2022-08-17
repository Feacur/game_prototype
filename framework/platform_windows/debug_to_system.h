#if !defined(FRAMEWORK_PLATFORM_WINDOWS_DEBUG_TO_SYSTEM)
#define FRAMEWORK_PLATFORM_WINDOWS_DEBUG_TO_SYSTEM

// interface from `debug.c` to `system.c`
// - framework initialization

#include "framework/common.h"

bool debug_to_system_init(void);
void debug_to_system_free(void);

#endif
