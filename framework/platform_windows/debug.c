#if !defined (GAME_TARGET_OPTIMIZED)
// ----- ----- ----- ----- -----
//     !GAME_TARGET_OPTIMIZED
// ----- ----- ----- ----- -----

#include "framework/logger.h"
#include "framework/memory.h"
#include "framework/containers/buffer.h"

#if defined (UNICODE) || defined (_UNICODE)
	#define DBGHELP_TRANSLATE_TCHAR
#endif

#include <Windows.h>
#include <DbgHelp.h>

static struct Platform_Debug {
	struct Buffer buffer;
	struct Buffer scratch;
} gs_platform_debug;

//
#include "framework/platform_debug.h"

struct Callstack platform_debug_get_callstack(void) {
	struct Callstack callstack = {0};

	CONTEXT context; RtlCaptureContext(&context);
	for (uint32_t i = 0; i < SIZE_OF_ARRAY(callstack.data); i++) {
		DWORD64 image_base;
		PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(context.Rip, &image_base, NULL);
		if (entry == NULL) { break; }

		callstack.data[callstack.count++] = context.Rip;

		PVOID handler_data;
		DWORD64 establisher_frame;
		RtlVirtualUnwind(UNW_FLAG_NHANDLER,
			image_base, context.Rip, entry,
			&context, &handler_data, &establisher_frame,
			NULL
		);
	}
	return callstack;

	// https://docs.microsoft.com/windows/win32/api/winnt/nf-winnt-rtlcapturecontext
	// https://docs.microsoft.com/windows/win32/api/winnt/nf-winnt-rtllookupfunctionentry
	// https://docs.microsoft.com/windows/win32/api/winnt/nf-winnt-rtlvirtualunwind
}

struct CString platform_debug_get_stacktrace(struct Callstack callstack, uint32_t offset) {
	gs_platform_debug.buffer.size = 0;
	gs_platform_debug.scratch.size = 0;

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

	HANDLE const process = GetCurrentProcess();
	for (uint32_t i = offset + 1, last = callstack.count; i < last; i++) {
		// fetch function, source file, and line
		BOOL const valid_symbol = SymFromAddr(process, callstack.data[i], NULL, &symbol.header);
		BOOL const valid_source = SymGetLineFromAddr64(process, callstack.data[i], &source_offset, &source) && (source.FileName != NULL);

	#if defined(DBGHELP_TRANSLATE_TCHAR)
		uint32_t const symbol_length = valid_symbol ? (uint32_t)WideCharToMultiByte(CP_UTF8, 0, symbol.header.Name, (int)symbol.header.NameLen, NULL, 0, NULL, NULL) : 0;
		uint32_t const source_length = valid_source ? (uint32_t)WideCharToMultiByte(CP_UTF8, 0, source.FileName, -1, NULL, 0, NULL, NULL) : 0;
		buffer_ensure(&gs_platform_debug.scratch, symbol_length + source_length);
		WideCharToMultiByte(CP_UTF8, 0, symbol.header.Name, (int)symbol.header.NameLen, gs_platform_debug.scratch.data, (int)symbol_length, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, source.FileName, -1, (char *)gs_platform_debug.scratch.data + symbol_length, (int)source_length, NULL, NULL);
		char const * symbol_data = gs_platform_debug.scratch.data;
		char const * source_data = (char *)gs_platform_debug.scratch.data + symbol_length;
	#else
		uint32_t const symbol_length = valid_symbol ? (uint32_t)symbol.header.NameLen : 0;
		uint32_t const source_length = valid_source ? (uint32_t)strlen(source.FileName) : 0;
		char const * symbol_data = symbol.header.Name;
		char const * source_data = source.FileName;
	#endif

		// reserve output buffer
		buffer_ensure(&gs_platform_debug.buffer, gs_platform_debug.buffer.size + 1 + symbol_length + 4 + source_length + 16);

		// fill the buffer
		uint32_t const written = logger_to_buffer(
			(uint32_t)(gs_platform_debug.buffer.capacity - gs_platform_debug.buffer.size),
			(char *)gs_platform_debug.buffer.data + gs_platform_debug.buffer.size,
			"%.*s at '%.*s:%u'\n"
			""
			, symbol_length, symbol_data
			, source_length, source_data
			, (source_length > 0) ? (uint32_t)source.LineNumber : 0
		);
		gs_platform_debug.buffer.size += written;

		struct CString const cs_symbol = {.length = symbol_length, .data = symbol_data};
		if (cstring_equals(cs_symbol, S_("main"))) { break; }
	}

	return (struct CString){
		.length = (uint32_t)gs_platform_debug.buffer.size,
		.data = (gs_platform_debug.buffer.size > 0)
			? gs_platform_debug.buffer.data
			: NULL,
	};

	// https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
	// https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symgetlinefromaddr64
}

//
#include "internal/debug_to_system.h"

bool debug_to_system_init(void) {
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	gs_platform_debug = (struct Platform_Debug){
		.buffer  = buffer_init(memory_reallocate_without_tracking),
		.scratch = buffer_init(memory_reallocate_without_tracking),
	};
	buffer_ensure(&gs_platform_debug.buffer,  4096);
	buffer_ensure(&gs_platform_debug.scratch, 512);

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
//     GAME_TARGET_OPTIMIZED
// ----- ----- ----- ----- -----

//
#include "framework/platform_debug.h"

struct Callstack platform_debug_get_callstack(void) { return (struct Callstack){0}; }
struct CString platform_debug_get_stacktrace(struct Callstack callstack, uint32_t offset) { (void)callstack; (void)offset; return (struct CString){0}; }

//
#include "debug_to_system.h"

bool debug_to_system_init(void) { return true; }
void debug_to_system_free(void) { }

#endif
