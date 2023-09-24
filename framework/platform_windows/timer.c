#include "framework/common.h"
#include "framework/logger.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>

static struct Platform_Timer {
	LARGE_INTEGER ticks_per_second;
} gs_platform_timer;

//
#include "framework/platform_timer.h"

uint64_t platform_timer_get_ticks(void) {
	LARGE_INTEGER ticks;
	return QueryPerformanceCounter(&ticks)
		? (uint64_t)ticks.QuadPart
		: 0;
}

uint64_t platform_timer_get_ticks_per_second(void) {
	return (uint64_t)gs_platform_timer.ticks_per_second.QuadPart;
}

//
#include "internal/timer_to_system.h"

bool timer_to_system_init(void) {
	return QueryPerformanceFrequency(&gs_platform_timer.ticks_per_second);
}

void timer_to_system_free(void) {
	common_memset(&gs_platform_timer, 0, sizeof(gs_platform_timer));
}
