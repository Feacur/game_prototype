#include "timer_to_system.h"
#include "window_to_system.h"
#include "glibrary_to_system.h"

#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct {
	HMODULE module;
} platform_system;

//
#include "code/platform_system.h"

static void set_process_dpi_awareness(void);
void platform_system_init(void) {
	platform_system.module = GetModuleHandleA(NULL);
	if (platform_system.module == NULL) { fprintf(stderr, "'GetModuleHandle' failed\n"); DEBUG_BREAK(); exit(1); }

	set_process_dpi_awareness();

	timer_to_system_init();
	window_to_system_init();
	glibrary_to_system_init();
}

void platform_system_free(void) {
	glibrary_to_system_free();
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

//

#include <ShellScalingApi.h>

static void set_process_dpi_awareness(void) {
#define FREE_DPI_LIBS() \
	do { \
		if (User32_dll != NULL) { FreeLibrary(User32_dll); User32_dll = NULL; } \
		if (Shcore_dll != NULL) { FreeLibrary(Shcore_dll); Shcore_dll = NULL; } \
	} while (false)\

	HMODULE User32_dll = LoadLibraryA("User32.dll");
	HMODULE Shcore_dll = LoadLibraryA("Shcore.dll");

	// Windows 10, version 1607
	if (User32_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(DPI_AWARENESS_CONTEXT);
		set_awareness_func set_awareness = (set_awareness_func)GetProcAddress(User32_dll, "SetProcessDpiAwarenessContext");
		if (set_awareness != NULL) {
			set_awareness(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			FREE_DPI_LIBS();
		}
	}

	// Windows 8.1
	if (Shcore_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(PROCESS_DPI_AWARENESS);
		set_awareness_func set_awareness = (set_awareness_func)GetProcAddress(Shcore_dll, "SetProcessDpiAwareness");
		if (set_awareness != NULL) {
			set_awareness(PROCESS_PER_MONITOR_DPI_AWARE);
			FREE_DPI_LIBS();
		}
	}

	// Windows Vista
	if (User32_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(VOID);
		set_awareness_func set_awareness = (set_awareness_func)GetProcAddress(User32_dll, "SetProcessDPIAware");
		if (set_awareness != NULL) {
			set_awareness();
			FREE_DPI_LIBS();
		}
	}

	FREE_DPI_LIBS();

	// https://docs.microsoft.com/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process

#undef FREE_DPI_LIBS
}
