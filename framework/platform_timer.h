#if !defined(FRAMEWORK_PLATFORM_TIMER)
#define FRAMEWORK_PLATFORM_TIMER

#include "common.h"

uint64_t platform_timer_get_ticks(void);
uint64_t platform_timer_get_ticks_per_second(void);

#endif
