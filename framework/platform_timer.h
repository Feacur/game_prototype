#if !defined(GAME_PLATFORM_TIMER)
#define GAME_PLATFORM_TIMER

#include "common.h"

uint64_t platform_timer_get_ticks(void);
uint64_t platform_timer_get_ticks_per_second(void);

#endif
