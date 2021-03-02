#include "code/common.h"

#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct {
	LARGE_INTEGER ticks_per_second;
} platform_timer;

//
#include "code/platform_timer.h"

uint64_t platform_timer_get_ticks(void) {
	LARGE_INTEGER ticks;
	if (!QueryPerformanceCounter(&ticks)) {
		fprintf(stderr, "'QueryPerformanceCounter' failed\n"); DEBUG_BREAK(); exit(1);
	}
	return (uint64_t)ticks.QuadPart;
}

uint64_t platform_timer_get_ticks_per_second(void) {
	return (uint64_t)platform_timer.ticks_per_second.QuadPart;
}

//
#include "timer_to_system.h"

void timer_to_system_init(void) {
	if (!QueryPerformanceFrequency(&platform_timer.ticks_per_second)) {
		fprintf(stderr, "'QueryPerformanceFrequency' failed\n"); DEBUG_BREAK(); exit(1);
	}
}

void timer_to_system_free(void) {
	memset(&platform_timer, 0, sizeof(platform_timer));
}
