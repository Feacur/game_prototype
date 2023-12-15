#include "framework/common.h"
#include "framework/formatter.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>


//
#include "framework/platform/timer.h"

uint64_t platform_timer_get_ticks(void) {
	LARGE_INTEGER ticks;
	return QueryPerformanceCounter(&ticks)
		? (uint64_t)ticks.QuadPart
		: 0;
}

uint64_t platform_timer_get_ticks_per_second(void) {
	static LARGE_INTEGER frequency = {0};
	if (frequency.QuadPart == 0) {
		QueryPerformanceFrequency(&frequency);
		TRC("> timer: update frequency to %lld", frequency.QuadPart);
	}
	return (uint64_t)frequency.QuadPart;
}
