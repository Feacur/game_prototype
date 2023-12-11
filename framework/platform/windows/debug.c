#if !defined (GAME_TARGET_RELEASE)
// ----- ----- ----- ----- -----
//     !GAME_TARGET_RELEASE
// ----- ----- ----- ----- -----

#include "framework/formatter.h"
#include "framework/memory.h"
#include "framework/systems/arena_system.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>
#include <DbgHelp.h>

static struct Platform_Debug {
	bool init;
} gs_platform_debug;

// @note: tested for `AMD64`; the whole thing depends heavily
//        on the platform; might require dynamic dll calls
//        instead of statically chosen at compile-time (?)

//
#include "framework/platform/debug.h"

struct Callstack platform_debug_get_callstack(uint32_t skip) {
	skip += 1; // @note: skip `platform_debug_get_callstack`

	uint32_t count = 0;
	struct Callstack callstack = {0};
	CONTEXT context; RtlCaptureContext(&context); // @note: platform-specific structure
	for (uint32_t i = 0; i < SIZE_OF_ARRAY(callstack.data); i++) {
		DWORD64 const control_pc = context.Rip; // @note: platform-specific field

		DWORD64 image_base;
		PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(control_pc, &image_base, NULL);
		if (entry == NULL) { break; }

		if (skip == 0) {
			callstack.data[count++] = control_pc;
		}
		else { skip--; }

		PVOID handler_data;
		DWORD64 establisher_frame;
		RtlVirtualUnwind(UNW_FLAG_NHANDLER,
			image_base, control_pc, entry,
			&context, &handler_data, &establisher_frame,
			NULL
		);
	}
	return callstack;

	// https://learn.microsoft.com/windows/win32/api/winnt/nf-winnt-rtlcapturecontext
	// https://learn.microsoft.com/windows/win32/api/winnt/nf-winnt-rtllookupfunctionentry
	// https://learn.microsoft.com/windows/win32/api/winnt/nf-winnt-rtlvirtualunwind
}

struct CString platform_debug_get_stacktrace(struct Callstack callstack) {
	static uint32_t const FRAMES_MAX = SIZE_OF_MEMBER(struct Callstack, data);

	if (!gs_platform_debug.init) {
		DEBUG_BREAK();
		return (struct CString){0};
	}

	DWORD64 symbol_offset = 0;
	union {
		SYMBOL_INFO header;
		uint8_t payload[sizeof(SYMBOL_INFO) + 1024];
	} symbol;
	symbol.header = (SYMBOL_INFO){
		.SizeOfStruct  = sizeof(symbol.header),
		.MaxNameLen = sizeof(symbol.payload) - sizeof(symbol.header),
	};

	DWORD source_offset = 0;
	IMAGEHLP_LINE64 source = {.SizeOfStruct = sizeof(source)};
	IMAGEHLP_MODULE64 module = {.SizeOfStruct = sizeof(module)};

	size_t size = 0;
	char * buffer = NULL;

	HANDLE const process = GetCurrentProcess();
	for (uint32_t i = 0; i < FRAMES_MAX && callstack.data[i] != 0; i++) {
		// fetch function, source file, and line
		BOOL const valid_module = SymGetModuleInfo64(process, callstack.data[i], &module);
		BOOL const valid_symbol = SymFromAddr(process, callstack.data[i], &symbol_offset, &symbol.header);
		BOOL const valid_source = SymGetLineFromAddr64(process, callstack.data[i], &source_offset, &source) && (source.FileName != NULL);
		if (!(valid_module || valid_symbol || valid_source)) { continue; }

		uint32_t const module_length = valid_module ? (uint32_t)strlen(module.ModuleName) : 0;
		uint32_t const symbol_length = valid_symbol ? (uint32_t)symbol.header.NameLen     : 0;
		uint32_t const source_length = valid_source ? (uint32_t)strlen(source.FileName)   : 0;
		char const * module_data = module.ModuleName;
		char const * symbol_data = symbol.header.Name;
		char const * source_data = source.FileName;

		// reserve output buffer
		size_t const capacity = size
			+ module_length + symbol_length + source_length
			+ 34 // chars
			+ 11 // UINT32_MAX
			+ 11 // UINT32_MAX
			;
		buffer = arena_reallocate(buffer, capacity * sizeof(char));

		// fill the buffer
		size += formatter_fmt(
			(uint32_t)(capacity - size),
			buffer + size,
			"[%.*s] %.*s at '%.*s:%u:%u'\n"
			""
			, module_length, module_data
			, symbol_length, symbol_data
			, source_length, source_data
			, (uint32_t)source.LineNumber
			, (uint32_t)source_offset
		);

		struct CString const cs_symbol = {.length = symbol_length, .data = symbol_data};
		if (cstring_equals(cs_symbol, S_("main"))) { break; }
	}

	if (size > 0) { // @note: strip last LF
		if (buffer[size - 1] == '\n') { size--; }
	}

	return (struct CString){
		.length = (uint32_t)size,
		.data = buffer,
	};

	// https://learn.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
	// https://learn.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symgetlinefromaddr64
}

//
#include "internal/debug_to_system.h"

bool debug_to_system_init(void) {
	gs_platform_debug = (struct Platform_Debug){
		.init = SymInitialize(GetCurrentProcess(), NULL, TRUE),
	};
	return true;
}

void debug_to_system_free(void) {
	SymCleanup(GetCurrentProcess());
	common_memset(&gs_platform_debug, 0, sizeof(gs_platform_debug));
}

#else
// ----- ----- ----- ----- -----
//     GAME_TARGET_RELEASE
// ----- ----- ----- ----- -----

//
#include "framework/platform/debug.h"

struct Callstack platform_debug_get_callstack(uint32_t skip) { (void)skip; return (struct Callstack){0}; }
struct CString platform_debug_get_stacktrace(struct Callstack callstack) { (void)callstack; return (struct CString){0}; }

//
#include "internal/debug_to_system.h"

bool debug_to_system_init(void) { return true; }
void debug_to_system_free(void) { }

#endif
