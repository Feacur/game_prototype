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
	HANDLE vectored;

	bool has_error;
} gs_platform_system;

// static void system_cache_paths(void);
static void system_set_process_dpi_awareness(void);
// static void system_enable_virtual_terminal_processing(void);
static void system_signal_handler(int value);
static LONG WINAPI system_vectored_handler(EXCEPTION_POINTERS * ExceptionInfo);
void platform_system_init(struct Platform_Callbacks callbacks) {
	signal(SIGABRT, system_signal_handler);
	signal(SIGFPE,  system_signal_handler);
	signal(SIGILL,  system_signal_handler);
	signal(SIGINT,  system_signal_handler);
	signal(SIGSEGV, system_signal_handler);
	signal(SIGTERM, system_signal_handler);

	gs_platform_system = (struct Platform_System){
		.callbacks = callbacks,
		.module   = GetModuleHandle(NULL),
		.process  = GetCurrentProcess(),
		.thread   = GetCurrentThread(),
		.vectored = AddVectoredExceptionHandler(0, system_vectored_handler),
	};
	if (gs_platform_system.module   == NULL) { goto fail; }
	if (gs_platform_system.process  == NULL) { goto fail; }
	if (gs_platform_system.thread   == NULL) { goto fail; }
	if (gs_platform_system.vectored == NULL) { goto fail; }

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

	if (!debug_to_system_init())       { goto fail; }
	if (!timer_to_system_init())       { goto fail; }
	if (!window_to_system_init())      { goto fail; }
	if (!gpu_library_to_system_init()) { goto fail; }
	if (!input_to_system_init())       { goto fail; }

	return;

	// process errors
	fail: DEBUG_BREAK();
	logger_to_console("failed to initialize the system module\n");
	gs_platform_system.has_error = true;
	// common_exit_failure();
}

void platform_system_free(void) {
	input_to_system_free();
	gpu_library_to_system_free();
	window_to_system_free();
	timer_to_system_free();

	if (memory_to_system_cleanup()) { DEBUG_BREAK(); }
	debug_to_system_free();

	RemoveVectoredContinueHandler(gs_platform_system.vectored);
	common_memset(&gs_platform_system, 0, sizeof(gs_platform_system));

	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);
	signal(SIGILL,  SIG_DFL);
	signal(SIGINT,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
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
	if (User32_dll != NULL) { FreeLibrary(User32_dll); }
	if (Shcore_dll != NULL) { FreeLibrary(Shcore_dll); }

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
#define STACKTRACE_OFFSET 7 // or 1

	if (gs_platform_system.has_error) { goto finilize; }

	logger_to_console(
		"> system signal '0x%x'\n"
		"  type: %s\n"
		""
		, value
		, system_signal_get_type(value)
	);
	REPORT_CALLSTACK(STACKTRACE_OFFSET); DEBUG_BREAK();

	gs_platform_system.has_error = true;

	finilize:
	(void)0;

	// http://www.cplusplus.com/reference/csignal/signal/
#undef STACKTRACE_OFFSET
}

//

static char const * system_vector_get_type(DWORD value) {
	switch (value) {
		case EXCEPTION_ACCESS_VIOLATION:         return "access violation";         // The thread tried to read from or write to a virtual address for which it does not have the appropriate access.
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "array bounds exceeded";    // The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.
		case EXCEPTION_BREAKPOINT:               return "breakpoint";               // A breakpoint was encountered.
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "datatype misalignment";    // The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return "flt denormal operand";     // One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "flt divide by zero";       // The thread tried to divide a floating-point value by a floating-point divisor of zero.
		case EXCEPTION_FLT_INEXACT_RESULT:       return "flt inexact result";       // The result of a floating-point operation cannot be represented exactly as a decimal fraction.
		case EXCEPTION_FLT_INVALID_OPERATION:    return "flt invalid operation";    // This exception represents any floating-point exception not included in this list.
		case EXCEPTION_FLT_OVERFLOW:             return "flt overflow";             // The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.
		case EXCEPTION_FLT_STACK_CHECK:          return "flt stack check";          // The stack overflowed or underflowed as the result of a floating-point operation.
		case EXCEPTION_FLT_UNDERFLOW:            return "flt underflow";            // The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "illegal instruction";      // The thread tried to execute an invalid instruction.
		case EXCEPTION_IN_PAGE_ERROR:            return "in page error";            // The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "int divide by zero";       // The thread tried to divide an integer value by an integer divisor of zero.
		case EXCEPTION_INT_OVERFLOW:             return "int overflow";             // The result of an integer operation caused a carry out of the most significant bit of the result.
		case EXCEPTION_INVALID_DISPOSITION:      return "invalid disposition";      // An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "noncontinuable exception"; // The thread tried to continue execution after a noncontinuable exception occurred.
		case EXCEPTION_PRIV_INSTRUCTION:         return "priv instruction";         // The thread tried to execute an instruction whose operation is not allowed in the current machine mode.
		case EXCEPTION_SINGLE_STEP:              return "single step";              // A trace trap or other single-instruction mechanism signaled that one instruction has been executed.
		case EXCEPTION_STACK_OVERFLOW:           return "stack overflow";           // The thread used up its stack.
	}
	return "unknown";
}

static LONG WINAPI system_vectored_handler(EXCEPTION_POINTERS * ExceptionInfo) {
#define STACKTRACE_OFFSET 4 // or 1

	DWORD const code = ExceptionInfo->ExceptionRecord->ExceptionCode;
	if (gs_platform_system.has_error) { goto finilize; }
	if (code == EXCEPTION_BREAKPOINT) { goto finilize; }

	// bool const noncontinuable = (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == EXCEPTION_NONCONTINUABLE;
	// bool const software_originate = (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_SOFTWARE_ORIGINATE) == EXCEPTION_SOFTWARE_ORIGINATE;

	// switch (code)
	// {
	// 	case EXCEPTION_ACCESS_VIOLATION: {
	// 		ULONGLONG const rw_flag = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
	// 		ULONGLONG const address = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
	// 	} break;
	// 
	// 	case EXCEPTION_IN_PAGE_ERROR: {
	// 		ULONGLONG const rw_flag   = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
	// 		ULONGLONG const address   = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
	// 		ULONGLONG const nt_status = ExceptionInfo->ExceptionRecord->ExceptionInformation[2];
	// 	} break;
	// }

	logger_to_console(
		"> system vector '0x%lx'\n"
		"  type: %s\n"
		""
		, code
		, system_vector_get_type(code)
	);
	REPORT_CALLSTACK(STACKTRACE_OFFSET); DEBUG_BREAK();

	gs_platform_system.has_error = true;

	finilize:
	// return EXCEPTION_CONTINUE_SEARCH;
	return EXCEPTION_CONTINUE_EXECUTION;

	// https://learn.microsoft.com/windows/win32/api/winnt/ns-winnt-exception_record
#undef STACKTRACE_OFFSET
}
