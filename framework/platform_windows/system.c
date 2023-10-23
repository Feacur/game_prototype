#include "framework/internal/input_to_system.h"
#include "framework/internal/memory_to_system.h"
#include "framework/containers/buffer.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/input_keys.h"
#include "framework/formatter.h"
#include "framework/platform_debug.h"

#include "internal/debug_to_system.h"
#include "internal/timer_to_system.h"
#include "internal/window_to_system.h"
#include "internal/gpu_library_to_system.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>
#include <signal.h>

// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
// Global Variable NvOptimusEnablement (new in Driver Release 302)
// Starting with the Release 302 drivers, application developers can direct the Optimus driver at runtime to use the High Performance Graphics to render any application–even those applications for which there is no existing application profile. They can do this by exporting a global variable named NvOptimusEnablement. The Optimus driver looks for the existence and value of the export. Only the LSB of the DWORD matters at this time. Avalue of 0x00000001 indicates that rendering should be performed using High Performance Graphics. A value of 0x00000000 indicates that this method should beignored.
__declspec(dllexport) extern DWORD NvOptimusEnablement;
DWORD NvOptimusEnablement = 1;

// https://community.amd.com/thread/169965
// https://community.amd.com/thread/223376
// https://gpuopen.com/amdpowerxpressrequesthighperformance
// Many Gaming and workstation laptops are available with both (1) integrated power saving and (2) discrete high performance graphics devices. Unfortunately, 3D intensive application performance may suffer greatly if the best graphics device is not selected. For example, a game may run at 30 Frames Per Second (FPS) on the integrated GPU rather than the 60 FPS the discrete GPU would enable. As a developer you can easily fix this problem by adding only one line to your executable’s source code:
// Yes, it’s that easy. This line will ensure that the high-performance graphics device is chosen when running your application.
__declspec(dllexport) extern DWORD AmdPowerXpressRequestHighPerformance;
DWORD AmdPowerXpressRequestHighPerformance = 1;

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

	{
		// -- terminal, code page
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		// -- terminal, virtual processing
		DWORD const handles[] = {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
		for (uint32_t i = 0; i < SIZE_OF_ARRAY(handles); i++) {
			HANDLE handle = GetStdHandle(handles[i]);
			if (handle == NULL) { continue; }
			DWORD mode = 0;
			if (GetConsoleMode(handle, &mode)) {
				SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			}
		}

		// -- threading
		// SetThreadAffinityMask(
		// 	gs_platform_system.module,
		// 	1 << GetCurrentProcessorNumber()
		// );
		// https://learn.microsoft.com/windows/win32/dxtecharts/game-timing-and-multicore-processors
	}

	if (!memory_to_system_init())      { goto fail; }
	if (!debug_to_system_init())       { goto fail; }
	if (!timer_to_system_init())       { goto fail; }
	if (!window_to_system_init())      { goto fail; }
	if (!gpu_library_to_system_init()) { goto fail; }
	if (!input_to_system_init())       { goto fail; }

	return;

	// process errors
	fail: ERR("failed to initialize the system module");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	gs_platform_system.has_error = true;
	// common_exit_failure();
}

void platform_system_free(void) {
	input_to_system_free();
	gpu_library_to_system_free();
	window_to_system_free();
	timer_to_system_free();
	debug_to_system_free();
	memory_to_system_free();

	RemoveVectoredContinueHandler(gs_platform_system.vectored);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);
	signal(SIGILL,  SIG_DFL);
	signal(SIGINT,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	common_memset(&gs_platform_system, 0, sizeof(gs_platform_system));
}

// bool platform_system_is_powered(void) {
// 	SYSTEM_POWER_STATUS system_power_status;
// 	GetSystemPowerStatus(&system_power_status);
// 	/*
// 	ACLineStatus: 0 or 1; 255 for unknown
// 	BatteryFlag: 1, 2, 4 (> 66%, < 33%, < 5%); 8 for charging; 128 for no battery; 255 for unknown
// 	BatteryLifePercent: [0 .. 100]; 255 for unknown
// 	SystemStatusFlag: 0 or 1 (power saving is off or on)
// 	BatteryLifeTime: seconds of lifetime remaining; -1 for unknown
// 	BatteryFullLifeTime: seconds of lifetime total; -1 for unknown
// 	*/
// 	return system_power_status.ACLineStatus == 1;
// 
// 	// https://learn.microsoft.com/windows/win32/api/winbase/ns-winbase-system_power_status
// 	// https://learn.microsoft.com/windows-hardware/design/component-guidelines/battery-saver
// }

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

// void platform_system_quit(void) {
// 	PostQuitMessage(0);
// }

void platform_system_sleep(uint32_t millis) {
	if (millis == 0) { YieldProcessor(); return; }
	Sleep(millis);
}

//
#include "internal/system.h"

void * system_to_internal_get_module(void) {
	return gs_platform_system.module;
}

//

static void log_last_error(char const * prefix) {
	DWORD id = GetLastError();
	if (id == 0) { return; }

	LPSTR message = NULL;
	DWORD size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
		, NULL, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
		, (LPSTR)&message, 0, NULL
	);
	if (size > 0 && message[size - 1] == '\n') { size -= 1; }
	if (prefix != NULL) { LOG("%s ", prefix); }
	LOG("%.*s\n", (int)size, message);
	LocalFree(message);
}

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
	if (gs_platform_system.has_error) { goto finilize; }

	LOG(
		"> system signal '0x%x'\n"
		"  type: %s\n"
		""
		, value
		, system_signal_get_type(value)
	);
	log_last_error("  message:");
	REPORT_CALLSTACK(); DEBUG_BREAK();

	gs_platform_system.has_error = true;

	finilize:
	(void)0;

	// http://www.cplusplus.com/reference/csignal/signal/
}

//

#define SVE_IGNORE_CLR (DWORD)0xe0434352 // CLR exception
#define SVE_IGNORE_CPP (DWORD)0xe06d7363 // C++ exception

#if !defined(MS_VC_EXCEPTION)
	#define MS_VC_EXCEPTION (DWORD)0x406d1388 // SetThreadName
	// https://learn.microsoft.com/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
#endif

#if !defined(DBG_UNABLE_TO_PROVIDE_HANDLE)
	#define DBG_UNABLE_TO_PROVIDE_HANDLE (DWORD) 0x40010002L
	// https://learn.microsoft.com/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
#endif

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
		// @note: there's a lot more, but MSDN describes a lot fewer; see `STATUS_*` in `winnt.h`
		case DBG_EXCEPTION_HANDLED:        return "dbg exception handled";        // Debugger handled the exception.
		case DBG_CONTINUE:                 return "dbg continue";                 // The debugger continued.
		case DBG_REPLY_LATER:              return "dbg reply later";              // Debugger will reply later.
		case DBG_UNABLE_TO_PROVIDE_HANDLE: return "dbg unable to provide handle"; // Debugger cannot provide a handle.
		case DBG_TERMINATE_THREAD:         return "dbg terminate thread";         // Debugger terminated the thread.
		case DBG_TERMINATE_PROCESS:        return "dbg terminate process";        // Debugger terminated the process.
		case DBG_CONTROL_C:                return "dbg control c";                // Debugger obtained control of C.
		case DBG_PRINTEXCEPTION_C:         return "dbg printexception c";         // Debugger printed an exception on control C.
		case DBG_RIPEXCEPTION:             return "dbg ripexception";             // Debugger received a RIP exception.
		case DBG_CONTROL_BREAK:            return "dbg control break";            // Debugger received a control break.
		case DBG_COMMAND_EXCEPTION:        return "dbg command exception";        // Debugger command communication exception.
		case DBG_PRINTEXCEPTION_WIDE_C:    return "dbg printexception wide c";    // Debugger obtained control of C.
		case DBG_EXCEPTION_NOT_HANDLED:    return "dbg exception not handled";    // Debugger did not handle the exception.
		//
		case MS_VC_EXCEPTION: return "SetThreadName";
		case SVE_IGNORE_CLR:  return "CLR exception";
		case SVE_IGNORE_CPP:  return "C++ exception";
	}
	return "unknown";
}

static LONG WINAPI system_vectored_handler(EXCEPTION_POINTERS * ExceptionInfo) {
	if (gs_platform_system.has_error) { return EXCEPTION_CONTINUE_EXECUTION; }
	DWORD const code = ExceptionInfo->ExceptionRecord->ExceptionCode;

	// @note: there are non-exception vectors as well
	// if (code == EXCEPTION_BREAKPOINT)  { return EXCEPTION_CONTINUE_SEARCH; }
	// if (code == EXCEPTION_SINGLE_STEP) { return EXCEPTION_CONTINUE_SEARCH; }

	// @note: ignore external exceptions
	if (code == MS_VC_EXCEPTION) { return EXCEPTION_CONTINUE_SEARCH; }
	if (code == SVE_IGNORE_CLR)  { return EXCEPTION_CONTINUE_SEARCH; }
	if (code == SVE_IGNORE_CPP)  { return EXCEPTION_CONTINUE_SEARCH; }

	// The presence of this flag indicates that the exception is a noncontinuable exception,
	// whereas the absence of this flag indicates that the exception is a continuable exception.
	// Any attempt to continue execution after a noncontinuable exception causes the EXCEPTION_NONCONTINUABLE_EXCEPTION exception.
	bool const noncontinuable = (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == EXCEPTION_NONCONTINUABLE;

	LOG(
		"> system vector '0x%lx'\n"
		"  type: %s\n"
		""
		, code
		, system_vector_get_type(code)
	);
	log_last_error("  message:");
	REPORT_CALLSTACK();

	if (noncontinuable) {
		DEBUG_BREAK();
		gs_platform_system.has_error = true;
	}

	return noncontinuable
		? EXCEPTION_CONTINUE_EXECUTION
		: EXCEPTION_CONTINUE_SEARCH;

	// https://learn.microsoft.com/windows/win32/api/winnt/ns-winnt-exception_record
	// https://learn.microsoft.com/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
	// https://wiki.winehq.org/Debugging_Hints

	// This flag is reserved for system use.
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
}

#undef SVE_IGNORE_CLR
#undef SVE_IGNORE_CPP

#if defined(GAME_ARCH_WINDOWS)
	#include <stdio.h>

	extern int main (int argc, char * argv[]);
	int WINAPI WinMain(
		HINSTANCE hInstance,     // The operating system uses this value to identify the executable or EXE when it's loaded in memory. Certain Windows functions need the instance handle, for example to load icons or bitmaps
		HINSTANCE hPrevInstance, // has no meaning. It was used in 16-bit Windows, but is now always zero
		PSTR      pCmdLine,      // contains the command-line arguments as a Unicode string.
		int       nCmdShow       // is a flag that indicates whether the main application window is minimized, maximized, or shown normally
	) {
		(void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;

		if (AllocConsole()) {
			FILE * file_stderr = NULL;
			freopen_s(&file_stderr, "CONOUT$", "w", stderr);

			FILE * file_stdout = NULL;
			freopen_s(&file_stdout, "CONOUT$", "w", stdout);
			
			FILE * file_stdin = NULL;
			freopen_s(&file_stdin, "CONIN$", "r", stdin);
		}

		return main(__argc, __argv);

		// @note: unicode differences
		// #if defined (UNICODE) || defined (_UNICODE)
		// 	wWinMain
		// 	PWSTR pCmdLine
		// 	__wargv
		// #endif

		// https://learn.microsoft.com/windows/win32/learnwin32/winmain--the-application-entry-point
		// https://learn.microsoft.com/windows/console/console-handles
	}
#endif
