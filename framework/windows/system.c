#include "framework/common.h"
#include "framework/internal/input_to_platform.h"

#include "timer_to_system.h"
#include "window_to_system.h"
#include "glibrary_to_system.h"

#include <Windows.h>
#include <signal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct Platform_System {
	HMODULE module;
	bool should_close;
} platform_system;

//
#include "framework/platform_system.h"

static void set_process_dpi_awareness(void);
static void system_signal_handler(int value);
void platform_system_init(void) {
	platform_system.module = GetModuleHandleA(NULL);
	if (platform_system.module == NULL) { fprintf(stderr, "'GetModuleHandle' failed\n"); DEBUG_BREAK(); exit(1); }

	set_process_dpi_awareness();

	signal(SIGABRT, system_signal_handler);
	signal(SIGFPE,  system_signal_handler);
	signal(SIGILL,  system_signal_handler);
	signal(SIGINT,  system_signal_handler);
	signal(SIGSEGV, system_signal_handler);
	signal(SIGTERM, system_signal_handler);

	timer_to_system_init();
	window_to_system_init();
	glibrary_to_system_init();
	input_to_platform_reset();
}

void platform_system_free(void) {
	input_to_platform_reset();
	glibrary_to_system_free();
	window_to_system_free();
	timer_to_system_free();
	memset(&platform_system, 0, sizeof(platform_system));
}

bool platform_window_is_running(void) {
	return !platform_system.should_close;
}

void platform_system_update(void) {
	MSG message;
	input_to_platform_before_update();
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
		if (message.message == WM_QUIT) {
			platform_system.should_close = true;
			DEBUG_BREAK();
			continue;
		}
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
	input_to_platform_after_update();
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

static void system_signal_handler(int value) {
	switch (value) {
		case SIGABRT: DEBUG_BREAK(); break; // Abnormal termination, such as is initiated by the abort function.
		case SIGFPE:  DEBUG_BREAK(); break; // Erroneous arithmetic operation, such as zero divide or an operation resulting in overflow (not necessarily with a floating-point operation).
		case SIGILL:  DEBUG_BREAK(); break; // Invalid function image, such as an illegal instruction. This is generally due to a corruption in the code or to an attempt to execute data.
		case SIGINT:  DEBUG_BREAK(); break; // Interactive attention signal. Generally generated by the application user.
		case SIGSEGV: DEBUG_BREAK(); break; // Invalid access to storage: When a program tries to read or write outside the memory it has allocated.
		case SIGTERM: DEBUG_BREAK(); break; // Termination request sent to program.
		default:      DEBUG_BREAK(); break; // ?
	}

	platform_system.should_close = true;

	// http://www.cplusplus.com/reference/csignal/signal/
}
