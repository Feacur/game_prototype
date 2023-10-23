#if !defined (GAME_TARGET_RELEASE)
// ----- ----- ----- ----- -----
//     !GAME_TARGET_RELEASE
// ----- ----- ----- ----- -----

#include "framework/formatter.h"
#include "framework/memory.h"
#include "framework/containers/buffer.h"

#if defined (UNICODE) || defined (_UNICODE)
	#define DBGHELP_TRANSLATE_TCHAR
#endif

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>
#include <DbgHelp.h>

static struct Platform_Debug {
	struct Buffer buffer;
	struct Buffer scratch;
} gs_platform_debug;

// @note: tested for `AMD64`; the whole thing depends heavily
//        on the platform; might require dynamic dll calls
//        instead of statically chosen at compile-time (?)

//
#include "framework/platform_debug.h"

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

#if defined(DBGHELP_TRANSLATE_TCHAR)
	inline static uint32_t platform_debug_eval_length(WCHAR const * value, int limit) {
		int const length = WideCharToMultiByte(CP_UTF8, 0, value, limit, NULL, 0, NULL, NULL);
		if (limit >= 0) { return (uint32_t)length; }
		if (length > 0) { return (uint32_t)(length - 1); }
		return 0;

		// https://learn.microsoft.com/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
	}
#endif

struct CString platform_debug_get_stacktrace(struct Callstack callstack) {
	static uint32_t const FRAMES_MAX = SIZE_OF_MEMBER(struct Callstack, data);

	gs_platform_debug.buffer.size = 0;
	gs_platform_debug.scratch.size = 0;

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

	HANDLE const process = GetCurrentProcess();
	for (uint32_t i = 0; i < FRAMES_MAX && callstack.data[i] > 0; i++) {
		// fetch function, source file, and line
		BOOL const valid_module = SymGetModuleInfo64(process, callstack.data[i], &module);
		BOOL const valid_symbol = SymFromAddr(process, callstack.data[i], &symbol_offset, &symbol.header);
		BOOL const valid_source = SymGetLineFromAddr64(process, callstack.data[i], &source_offset, &source) && (source.FileName != NULL);
		if (!(valid_module || valid_symbol || valid_source)) { continue; }

	#if defined(DBGHELP_TRANSLATE_TCHAR)
		uint32_t const module_length = valid_module ? platform_debug_eval_length(module.ModuleName, -1) : 0;
		uint32_t const symbol_length = valid_symbol ? platform_debug_eval_length(symbol.header.Name, (int)symbol.header.NameLen) : 0;
		uint32_t const source_length = valid_source ? platform_debug_eval_length(source.FileName, -1) : 0;
		buffer_ensure(&gs_platform_debug.scratch, module_length + symbol_length + source_length);
		char * module_data = gs_platform_debug.scratch.data;
		char * symbol_data = module_data + symbol_length;
		char * source_data = symbol_data + source_length;
		WideCharToMultiByte(CP_UTF8, 0, module.ModuleName, -1, module_data, (int)module_length, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, symbol.header.Name, (int)symbol.header.NameLen, symbol_data, (int)symbol_length, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, source.FileName, -1, source_data, (int)source_length, NULL, NULL);
	#else
		uint32_t const module_length = valid_module ? (uint32_t)strlen(source.ModuleName) : 0;
		uint32_t const symbol_length = valid_symbol ? (uint32_t)symbol.header.NameLen : 0;
		uint32_t const source_length = valid_source ? (uint32_t)strlen(source.FileName) : 0;
		char const * module_data = source.ModuleName;
		char const * symbol_data = symbol.header.Name;
		char const * source_data = source.FileName;
	#endif

		// reserve output buffer
		buffer_ensure(&gs_platform_debug.buffer,
			gs_platform_debug.buffer.size
			+ module_length + symbol_length + source_length
			+ 34 // chars
			+ 11 // UINT32_MAX
			+ 11 // UINT32_MAX
		);

		// fill the buffer
		uint32_t const written = formatter_fmt(
			(uint32_t)(gs_platform_debug.buffer.capacity - gs_platform_debug.buffer.size),
			(char *)gs_platform_debug.buffer.data + gs_platform_debug.buffer.size,
			"[%.*s] %.*s at '%.*s:%u:%u'\n"
			""
			, module_length, module_data
			, symbol_length, symbol_data
			, source_length, source_data
			, (uint32_t)source.LineNumber
			, (uint32_t)source_offset
		);
		gs_platform_debug.buffer.size += written;

		struct CString const cs_symbol = {.length = symbol_length, .data = symbol_data};
		if (cstring_equals(cs_symbol, S_("main"))) { break; }
	}

	if (gs_platform_debug.buffer.size > 0) {
		// @note: strip last LF
		char const * last = buffer_peek(&gs_platform_debug.buffer, 0);
		if (last[0] == '\n') { gs_platform_debug.buffer.size--; }
	}

	return (struct CString){
		.length = (uint32_t)gs_platform_debug.buffer.size,
		.data = (gs_platform_debug.buffer.size > 0)
			? gs_platform_debug.buffer.data
			: NULL,
	};

	// https://learn.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
	// https://learn.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symgetlinefromaddr64
}

//
#include "internal/debug_to_system.h"

bool debug_to_system_init(void) {
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	gs_platform_debug = (struct Platform_Debug){
		.buffer  = buffer_init(memory_reallocate_without_tracking),
		.scratch = buffer_init(memory_reallocate_without_tracking),
	};
	buffer_resize(&gs_platform_debug.buffer,  4096);
	buffer_resize(&gs_platform_debug.scratch, 512);

	return true;
}

void debug_to_system_free(void) {
	buffer_free(&gs_platform_debug.buffer);
	buffer_free(&gs_platform_debug.scratch);
	common_memset(&gs_platform_debug, 0, sizeof(gs_platform_debug));

	SymCleanup(GetCurrentProcess());
}

#else
// ----- ----- ----- ----- -----
//     GAME_TARGET_RELEASE
// ----- ----- ----- ----- -----

//
#include "framework/platform_debug.h"

struct Callstack platform_debug_get_callstack(uint32_t skip) { (void)skip; return (struct Callstack){0}; }
struct CString platform_debug_get_stacktrace(struct Callstack callstack) { (void)callstack; return (struct CString){0}; }

//
#include "internal/debug_to_system.h"

bool debug_to_system_init(void) { return true; }
void debug_to_system_free(void) { }

#endif
