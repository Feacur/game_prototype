#if !defined (GAME_TARGET_OPTIMIZED)
// ----- ----- ----- ----- -----
//     !GAME_TARGET_OPTIMIZED
// ----- ----- ----- ----- -----

#include "framework/logger.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <stdlib.h>

static struct Platform_Debug {
	struct Platform_Debug_Buffer {
		uint32_t capacity, count;
		void * data;
	} buffer;
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
	gs_platform_debug.buffer.count = 0;

	union {
		SYMBOL_INFO header;
		uint8_t payload[sizeof(SYMBOL_INFO) + 1024];
	} symbol;
	symbol.header = (SYMBOL_INFO){
		.SizeOfStruct  = sizeof(symbol.header),
		.MaxNameLen = sizeof(symbol.payload) - sizeof(symbol.header),
	};

	DWORD source_offset = 0;
	IMAGEHLP_LINE64 source = {.SizeOfStruct = sizeof(source),};

	HANDLE const process = GetCurrentProcess();
	for (uint32_t i = offset; i < callstack.count; i++) {
		// fetch source file, line, and function
		uint32_t symbol_length = 0;
		if (SymFromAddr(process, callstack.data[i], NULL, &symbol.header)) {
			symbol_length = symbol.header.NameLen;
		}

		uint32_t source_length = 0;
		if (SymGetLineFromAddr64(process, callstack.data[i], &source_offset, &source)) {
			source_length = (uint32_t)strlen(source.FileName);
		}

		// reserve output buffer
		uint32_t const estimated_length = 1 + symbol_length + 4 + source_length + 16;
		while (gs_platform_debug.buffer.count + estimated_length > gs_platform_debug.buffer.capacity) {
			uint32_t const next_capacity = gs_platform_debug.buffer.capacity * 2;
			void * reallocated = realloc(gs_platform_debug.buffer.data, next_capacity);
			if (reallocated == NULL) { continue; }
			gs_platform_debug.buffer.data = reallocated;
			gs_platform_debug.buffer.capacity = next_capacity;
		}
		if (gs_platform_debug.buffer.count + estimated_length > gs_platform_debug.buffer.capacity) { break; }

		// fill the buffer
		uint32_t const written = logger_to_buffer(
			gs_platform_debug.buffer.capacity - gs_platform_debug.buffer.count,
			(char *)gs_platform_debug.buffer.data + gs_platform_debug.buffer.count,
			"%.*s at '%.*s:%u'\n",
			symbol_length, symbol.header.Name,
			source_length, source.FileName,
			(source_length > 0) ? (uint32_t)source.LineNumber : 0
		);
		gs_platform_debug.buffer.count += written;
	}

	return (struct CString){
		.length = gs_platform_debug.buffer.count,
		.data = (gs_platform_debug.buffer.count > 0)
			? gs_platform_debug.buffer.data
			: NULL,
	};

	// https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
	// https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-symgetlinefromaddr64
}

//
#include "debug_to_system.h"

bool debug_to_system_init(void) {
#define INITIAL_CAPACITY 4096

	gs_platform_debug = (struct Platform_Debug){
		.buffer = {
			.capacity = INITIAL_CAPACITY,
			.data = malloc(INITIAL_CAPACITY),
		},
	};
	SymInitialize(GetCurrentProcess(), NULL, TRUE);
	return true;

#undef INITIAL_CAPACITY
}

void debug_to_system_free(void) {
	SymCleanup(GetCurrentProcess());
	free(gs_platform_debug.buffer.data);
	common_memset(&gs_platform_debug, 0, sizeof(gs_platform_debug));
}

#else
// ----- ----- ----- ----- -----
//     GAME_TARGET_OPTIMIZED
// ----- ----- ----- ----- -----

//
#include "framework/platform_debug.h"

struct Callstack platform_debug_get_callstack(void) { return (struct Callstack){0}; }
struct CString platform_debug_get_stacktrace(struct Callstack callstack, uint32_t offset) { (void)callstack; (void)offset; return S_NULL; }

//
#include "debug_to_system.h"

bool debug_to_system_init(void) { return true; }
void debug_to_system_free(void) { }

#endif

/*
{
	//
	// this is a significantly slower method
	//

	HANDLE const process = GetCurrentProcess();
	HANDLE const thread  = GetCurrentThread();

	// @note: for another thread
	// CONTEXT context = {.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER,};
	// if (SuspendThread(thread) == (DWORD)-1) { return stacktrace; }
	// if (!GetThreadContext(thread, &context)) { return stacktrace; }
	// if (ResumeThread(thread) == (DWORD)-1) { return stacktrace; }

	STACKFRAME64 stackframe = debug_get_stackframe(context);
	for (uint32_t i = 0; i < SIZE_OF_ARRAY(callstack.data); i++) {
		if (!StackWalk64(
			debug_get_machine_type(),
			process, thread,
			&stackframe, &context,
			NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL
		)) { break; }
		if (stackframe.AddrPC.Offset == 0) { break; }
		callstack.data[callstack.count++] = stackframe.AddrPC.Offset;
	}
}

//
// this are utilities for the commented code above
//

inline static DWORD debug_get_machine_type(void) {
#if defined(_X86_)
	return IMAGE_FILE_MACHINE_I386;
#elif defined(_IA64_)
	return IMAGE_FILE_MACHINE_IA64;
#elif defined(_AMD64_)
	return IMAGE_FILE_MACHINE_AMD64;
#else
	return 0;
#endif

	// https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64
	// https://docs.microsoft.com/cpp/preprocessor/predefined-macros
	// https://docs.microsoft.com/archive/blogs/reiley/macro-revisited
}

inline static STACKFRAME64 debug_get_stackframe(CONTEXT context) {
	STACKFRAME64 result = (STACKFRAME64){
		.AddrPC.Mode    = AddrModeFlat,
		.AddrStack.Mode = AddrModeFlat,
		.AddrFrame.Mode = AddrModeFlat,
	};

#if defined(_X86_)
	result.AddrPC.Offset    = context.Eip;
	result.AddrStack.Offset = context.Esp;
	result.AddrFrame.Offset = context.Ebp;
#elif defined(_IA64_)
	result.AddrPC.Offset     = context.StIIP;
	result.AddrStack.Offset  = context.IntSp;
	result.AddrBStore.Offset = context.RsBSP;
#elif defined(_AMD64_)
	result.AddrPC.Offset    = context.Rip;
	result.AddrStack.Offset = context.Rsp;
	result.AddrFrame.Offset = context.Rsp;
#endif

	return result;

	// https://docs.microsoft.com/windows/win32/api/dbghelp/ns-dbghelp-stackframe
}
*/
