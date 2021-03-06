#include "framework/internal/input_to_system.h"
#include "framework/internal/memory_to_system.h"
#include "framework/containers/array_byte.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/input_keys.h"
#include "framework/logger.h"

#include "timer_to_system.h"
#include "window_to_system.h"
#include "glibrary_to_system.h"

#include <Windows.h>
#include <signal.h>

#include <string.h>
#include <stdlib.h>

static struct Platform_System {
	HMODULE module;
	bool should_close;
} platform_system;

//
#include "framework/platform_system.h"

// static void system_cache_paths(void);
static void system_set_process_dpi_awareness(void);
// static void system_enable_virtual_terminal_processing(void);
static void system_signal_handler(int value);
void platform_system_init(void) {
	platform_system.module = GetModuleHandle(NULL);
	if (platform_system.module == NULL) { logger_to_console("'GetModuleHandle' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }

	// system_cache_paths();
	system_set_process_dpi_awareness();
	// system_enable_virtual_terminal_processing();
	SetConsoleOutputCP(CP_UTF8);

	signal(SIGABRT, system_signal_handler);
	signal(SIGFPE,  system_signal_handler);
	signal(SIGILL,  system_signal_handler);
	signal(SIGINT,  system_signal_handler);
	signal(SIGSEGV, system_signal_handler);
	signal(SIGTERM, system_signal_handler);

	memory_to_system_init();
	timer_to_system_init();
	window_to_system_init();
	glibrary_to_system_init();
	input_to_system_init();
}

void platform_system_free(void) {
	input_to_system_free();
	glibrary_to_system_free();
	window_to_system_free();
	timer_to_system_free();
	memory_to_system_free();
	memset(&platform_system, 0, sizeof(platform_system));
}

bool platform_window_is_running(void) {
	return !platform_system.should_close;
}

void platform_system_update(void) {
	MSG message;
	input_to_platform_before_update();
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
		if (message.message == WM_QUIT) {
			platform_system.should_close = true;
			DEBUG_BREAK();
			continue;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// high-order is state, low-order is toggle
	// 1) SHORT const caps_lock = GetKeyState(VK_CAPITAL);
	//    SHORT const num_lock = GetKeyState(VK_NUMLOCK);
	// 2) BYTE key_states[256];
	//    GetKeyboardState(key_states);

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

static void system_set_process_dpi_awareness(void) {
#define FREE_DPI_LIBS() \
	do { \
		if (User32_dll != NULL) { FreeLibrary(User32_dll); User32_dll = NULL; } \
		if (Shcore_dll != NULL) { FreeLibrary(Shcore_dll); Shcore_dll = NULL; } \
	} while (false) \

	HMODULE User32_dll = LoadLibrary(TEXT("User32.dll"));
	HMODULE Shcore_dll = LoadLibrary(TEXT("Shcore.dll"));

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

//
//
// static void system_enable_virtual_terminal_processing(void) {
// 	DWORD const handles[] = {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
// 	for (uint32_t i = 0; i < sizeof(handles) / sizeof(*handles); i++) {
// 		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
// 		if (handle == NULL) { continue; }
// 		DWORD mode = 0;
// 		if (GetConsoleMode(handle, &mode)) {
// 			SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
// 		}
// 	}
// }

//
// #include <ShlObj.h>
// 
// static void system_cache_paths(void) {
// 	// requires shell32.lib ole32.lib
// 
// 	PWSTR buffer_utf16 = NULL;
// 	if (SHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &buffer_utf16) != S_OK) { return; }
// 
// 	uint32_t length_utf16 = 0;
// 	while (buffer_utf16[length_utf16] != 0) { length_utf16++; }
// 	
// 	struct Array_Byte buffer_utf8;
// 	array_byte_init(&buffer_utf8);
// 	array_byte_resize(&buffer_utf8, length_utf16 * 4);
// 
// 	uint32_t utf16_high_surrogate = 0;
// 	for (uint16_t const * ptr = buffer_utf16; *ptr != 0; ptr++) {
// 		uint32_t value = *ptr;
// 
// 		// UTF-16 -> UTF-32
// 		if ((0xd800 <= value) && (value <= 0xdbff)) {
// 			// if (utf16_high_surrogate != 0) { DEBUG_BREAK(); utf16_high_surrogate = 0; return 0; }
// 			utf16_high_surrogate = value;
// 			continue;
// 		}
// 
// 		if ((0xdc00 <= value) && (value <= 0xdfff)) {
// 			// if (utf16_high_surrogate == 0) { DEBUG_BREAK(); return 0; }
// 			value = (((utf16_high_surrogate & 0x03ff) << 10) | (value & 0x03ff)) + 0x10000;
// 			utf16_high_surrogate = 0;
// 		}
// 
// 		// UTF-32 -> UTF-8
// 		if (value <= 0x0000007F) {
// 			array_byte_push(&buffer_utf8, (uint8_t)value);
// 		}
// 		else  if (0x00000080 <= value && value <= 0x000007FF) {
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >> 8) & 0x1f) | 0xc0);
// 			array_byte_push(&buffer_utf8, (uint8_t)(value        & 0x3f) | 0x80);
// 		}
// 		else if (0x00000800 <= value && value <= 0x0000FFFF) {
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >> 16) & 0x0f) | 0xe0);
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >>  8) & 0x3f) | 0x80);
// 			array_byte_push(&buffer_utf8, (uint8_t)(value         & 0x3f) | 0x80);
// 		}
// 		else if (0x00010000 <= value && value <= 0x0010FFFF) {
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >> 24) & 0x07) | 0xf0);
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >> 16) & 0x3f) | 0x80);
// 			array_byte_push(&buffer_utf8, (uint8_t)((value >>  8) & 0x3f) | 0x80);
// 			array_byte_push(&buffer_utf8, (uint8_t)(value         & 0x3f) | 0x80);
// 		}
// 		// else { logger_console("UTF-32 sequence is malformed\n"); DEBUG_BREAK(); }
// 	}
// 
// 	buffer_utf8.data[buffer_utf8.count] = '\0';
// 	array_byte_resize(&buffer_utf8, buffer_utf8.count + 1);
// 	array_byte_free(&buffer_utf8);
// 
// 	CoTaskMemFree(buffer_utf16);
// }

//

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
