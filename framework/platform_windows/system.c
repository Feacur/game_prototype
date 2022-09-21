#include "framework/internal/input_to_system.h"
#include "framework/internal/memory_to_system.h"
#include "framework/containers/buffer.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/input_keys.h"
#include "framework/logger.h"
#include "framework/platform_debug.h"

#include "internal/debug_to_system.h"
#include "internal/timer_to_system.h"
#include "internal/window_to_system.h"
#include "internal/gpu_library_to_system.h"

#include <Windows.h>
#include <signal.h>

//
#include "framework/platform_system.h"

static struct Platform_System {
	struct Platform_Callbacks callbacks;

	HMODULE module;
	HANDLE process;
	HANDLE thread;

	bool has_error;
} gs_platform_system;

// static void system_cache_paths(void);
static void system_set_process_dpi_awareness(void);
// static void system_enable_virtual_terminal_processing(void);
static void system_signal_handler(int value);
void platform_system_init(struct Platform_Callbacks callbacks) {
	signal(SIGABRT, system_signal_handler);
	signal(SIGFPE,  system_signal_handler);
	signal(SIGILL,  system_signal_handler);
	signal(SIGINT,  system_signal_handler);
	signal(SIGSEGV, system_signal_handler);
	signal(SIGTERM, system_signal_handler);

	gs_platform_system = (struct Platform_System){
		.callbacks = callbacks,
		.module = GetModuleHandle(NULL),
		.process = GetCurrentProcess(),
		.thread  = GetCurrentThread(),
	};
	if (gs_platform_system.module == NULL) { goto fail; }

	// @note: might as well lock main thread
	//        https://docs.microsoft.com/windows/win32/dxtecharts/game-timing-and-multicore-processors
	// SetThreadAffinityMask(
	// 	gs_platform_system.module,
	// 	1 << GetCurrentProcessorNumber()
	// );

	// system_cache_paths();
	system_set_process_dpi_awareness();
	// system_enable_virtual_terminal_processing();

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	if (!debug_to_system_init()) { goto fail; }
	if (!timer_to_system_init()) { goto fail; }
	if (!window_to_system_init()) { goto fail; }
	if (!gpu_library_to_system_init()) { goto fail; }
	if (!input_to_system_init()) { goto fail; }

	return;

	// process errors
	fail: DEBUG_BREAK();
	logger_to_console("failed to initialize the system module\n");
	gs_platform_system.has_error = true;
	// common_exit_failure();
}

void platform_system_free(void) {
	common_memset(&gs_platform_system, 0, sizeof(gs_platform_system));

	input_to_system_free();
	gpu_library_to_system_free();
	window_to_system_free();
	timer_to_system_free();

	if (memory_to_system_cleanup()) { DEBUG_BREAK(); }
	debug_to_system_free();
}

bool platform_system_is_powered(void) {
	SYSTEM_POWER_STATUS system_power_status;
	GetSystemPowerStatus(&system_power_status);
	/*
	ACLineStatus: 0 or 1; 255 for unknown
	BatteryFlag: 1, 2, 4 (> 66%, < 33%, < 5%); 8 for charging; 128 for no battery; 255 for unknown
	BatteryLifePercent: [0 .. 100]; 255 for unknown
	SystemStatusFlag: 0 or 1 (power saving is off or on)
	BatteryLifeTime: seconds of lifetime remaining; -1 for unknown
	BatteryFullLifeTime: seconds of lifetime total; -1 for unknown
	*/
	return system_power_status.ACLineStatus == 1;

	// https://docs.microsoft.com/windows/win32/api/winbase/ns-winbase-system_power_status
	// https://docs.microsoft.com/windows-hardware/design/component-guidelines/battery-saver
}

bool platform_system_is_error(void) {
	return gs_platform_system.has_error;
}

void platform_system_update(void) {
	input_to_platform_before_update();

	for (MSG message; PeekMessage(&message, NULL, 0, 0, PM_REMOVE); /*empty*/) {
		if (message.message == WM_QUIT) {
			if (gs_platform_system.callbacks.quit != NULL) {
				gs_platform_system.callbacks.quit();
			}
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

void platform_system_quit(void) {
	PostQuitMessage(0);
}

void platform_system_sleep(uint32_t millis) {
	// if (millis == 0) { YieldProcessor(); return; }
	Sleep(millis);
}

//
#include "internal/system.h"

void * system_to_internal_get_module(void) {
	return gs_platform_system.module;
}

//

#include <ShellScalingApi.h>

static void system_set_process_dpi_awareness(void) {
	HMODULE User32_dll = LoadLibrary(TEXT("User32.dll"));
	HMODULE Shcore_dll = LoadLibrary(TEXT("Shcore.dll"));

	// Windows 10, version 1607
	if (User32_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(DPI_AWARENESS_CONTEXT);
		set_awareness_func set_awareness = (set_awareness_func)(void *)GetProcAddress(User32_dll, "SetProcessDpiAwarenessContext");
		if (set_awareness != NULL) {
			set_awareness(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			goto finalize;
		}
	}

	// Windows 8.1
	if (Shcore_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(PROCESS_DPI_AWARENESS);
		set_awareness_func set_awareness = (set_awareness_func)(void *)GetProcAddress(Shcore_dll, "SetProcessDpiAwareness");
		if (set_awareness != NULL) {
			set_awareness(PROCESS_PER_MONITOR_DPI_AWARE);
			goto finalize;
		}
	}

	// Windows Vista
	if (User32_dll != NULL) {
		typedef BOOL (WINAPI * set_awareness_func)(VOID);
		set_awareness_func set_awareness = (set_awareness_func)(void *)GetProcAddress(User32_dll, "SetProcessDPIAware");
		if (set_awareness != NULL) {
			set_awareness();
			goto finalize;
		}
	}

	finalize:
	if (User32_dll != NULL) { FreeLibrary(User32_dll); User32_dll = NULL; }
	if (Shcore_dll != NULL) { FreeLibrary(Shcore_dll); Shcore_dll = NULL; }

	// https://docs.microsoft.com/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process
}

//
//
// static void system_enable_virtual_terminal_processing(void) {
// 	DWORD const handles[] = {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
// 	for (uint32_t i = 0; i < SIZE_OF_ARRAY(handles); i++) {
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
// 	struct Buffer buffer_utf8;
// 	buffer_init(&buffer_utf8);
// 	buffer_resize(&buffer_utf8, length_utf16 * 4);
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
// 			buffer_push(&buffer_utf8, (uint8_t)value);
// 		}
// 		else  if (0x00000080 <= value && value <= 0x000007FF) {
// 			buffer_push(&buffer_utf8, (uint8_t)((value >> 8) & 0x1f) | 0xc0);
// 			buffer_push(&buffer_utf8, (uint8_t)(value        & 0x3f) | 0x80);
// 		}
// 		else if (0x00000800 <= value && value <= 0x0000FFFF) {
// 			buffer_push(&buffer_utf8, (uint8_t)((value >> 16) & 0x0f) | 0xe0);
// 			buffer_push(&buffer_utf8, (uint8_t)((value >>  8) & 0x3f) | 0x80);
// 			buffer_push(&buffer_utf8, (uint8_t)(value         & 0x3f) | 0x80);
// 		}
// 		else if (0x00010000 <= value && value <= 0x0010FFFF) {
// 			buffer_push(&buffer_utf8, (uint8_t)((value >> 24) & 0x07) | 0xf0);
// 			buffer_push(&buffer_utf8, (uint8_t)((value >> 16) & 0x3f) | 0x80);
// 			buffer_push(&buffer_utf8, (uint8_t)((value >>  8) & 0x3f) | 0x80);
// 			buffer_push(&buffer_utf8, (uint8_t)(value         & 0x3f) | 0x80);
// 		}
// 		// else { logger_console("UTF-32 sequence is malformed\n"); DEBUG_BREAK(); }
// 	}
// 
// 	buffer_utf8.data[buffer_utf8.count] = '\0';
// 	buffer_resize(&buffer_utf8, buffer_utf8.count + 1);
// 	buffer_free(&buffer_utf8);
// 
// 	CoTaskMemFree(buffer_utf16);
// }

//

static char const * system_signal_get_type(int value) {
	switch (value) {
		case SIGABRT: return "abnormal termination";           // such as is initiated by the abort function.
		case SIGFPE:  return "erroneous arithmetic operation"; // such as zero divide or an operation resulting in overflow (not necessarily with a floating-point operation).
		case SIGILL:  return "invalid function image";         // such as an illegal instruction. This is generally due to a corruption in the code or to an attempt to execute data.
		case SIGINT:  return "interactive attention signal";   // generally generated by the application user.
		case SIGSEGV: return "invalid access to storage";      // when a program tries to read or write outside the memory it has allocated.
		case SIGTERM: return "termination request";            // sent to program.
	}
	return "unknown";
}

static void system_signal_handler(int value) {
#define STACKTRACE_OFFSET 7

	logger_to_console(
		"> System signal '0x%x'\n"
		"  type: %s\n"
		,
		value,
		system_signal_get_type(value)
	);
	REPORT_CALLSTACK(STACKTRACE_OFFSET); DEBUG_BREAK();

	gs_platform_system.has_error = true;

	// http://www.cplusplus.com/reference/csignal/signal/
#undef STACKTRACE_OFFSET
}
