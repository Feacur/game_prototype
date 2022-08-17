#if !defined(FRAMEWORK_PLATFORM_WINDOWS_TIMER_TO_SYSTEM)
#define FRAMEWORK_PLATFORM_WINDOWS_TIMER_TO_SYSTEM

// interface from `timer.c` to `system.c`
// - framework initialization

#include "framework/common.h"

bool timer_to_system_init(void);
void timer_to_system_free(void);

#endif
