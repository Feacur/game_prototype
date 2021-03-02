#include "timer_to_system.h"
#include "window_to_system.h"
#include "graphics_library.h"

#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct {
	HMODULE module;
} platform_system;

//
#include "code/platform_system.h"

void platform_system_init(void) {
	platform_system.module = GetModuleHandleA(NULL);
	if (platform_system.module == NULL) { fprintf(stderr, "'GetModuleHandle' failed\n"); DEBUG_BREAK(); exit(1); }
	timer_to_system_init();
	window_to_system_init();
	graphics_library_init();
}

void platform_system_free(void) {
	graphics_library_free();
	window_to_system_free();
	timer_to_system_free();
	memset(&platform_system, 0, sizeof(platform_system));
}

void platform_system_update(void) {
	MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
		if (message.message == WM_QUIT) { continue; }
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

void platform_system_sleep(uint32_t millis) {
	Sleep(millis);
}

//
#include "system_to_internal.h"

HMODULE system_to_internal_get_module(void) {
	return platform_system.module;
}
