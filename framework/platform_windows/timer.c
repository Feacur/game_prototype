#include "framework/common.h"
#include "framework/logger.h"

#include <Windows.h>

static struct Platform_Timer {
	LARGE_INTEGER ticks_per_second;
} gs_platform_timer;

//
#include "framework/platform_timer.h"

uint64_t platform_timer_get_ticks(void) {
	LARGE_INTEGER ticks;
	if (!QueryPerformanceCounter(&ticks)) {
		logger_to_console("'QueryPerformanceCounter' failed\n"); DEBUG_BREAK();
		common_exit_failure();
	}
	return (uint64_t)ticks.QuadPart;
}

uint64_t platform_timer_get_ticks_per_second(void) {
	return (uint64_t)gs_platform_timer.ticks_per_second.QuadPart;
}

//
#include "timer_to_system.h"

void timer_to_system_init(void) {
	if (!QueryPerformanceFrequency(&gs_platform_timer.ticks_per_second)) {
		logger_to_console("'QueryPerformanceFrequency' failed\n"); DEBUG_BREAK();
		common_exit_failure();
	}
}

void timer_to_system_free(void) {
	common_memset(&gs_platform_timer, 0, sizeof(gs_platform_timer));
}
