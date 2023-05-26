#include "framework/logger.h"
#include "framework/platform_debug.h"

#include <stdlib.h>

// @todo: custom specialized allocators
// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...
// @idea: use OS-native allocators instead of CRT's

//
#include "memory.h"

struct Memory_Header {
	size_t checksum, size;
	struct Memory_Header * prev, * next;
	struct Callstack callstack;
};

static struct Memory_Header * gs_memory;

void * memory_reallocate(void * pointer, size_t size) {
	struct Callstack const callstack = platform_debug_get_callstack();

	struct Memory_Header * pointer_header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (pointer_header != NULL) {
		if (pointer_header->checksum != (size_t)pointer_header) {
			struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
			logger_to_console("incorrect checksum: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
			return pointer;
		}

		pointer_header->checksum = 0;
		if (gs_memory == pointer_header) { gs_memory = pointer_header->next; }
		if (pointer_header->next != NULL) { pointer_header->next->prev = pointer_header->prev; }
		if (pointer_header->prev != NULL) { pointer_header->prev->next = pointer_header->next; }
	}

	// free
	if (size == 0) {
		if (pointer_header == NULL) { return NULL; }
		common_memset(pointer_header, 0, sizeof(*pointer_header) + pointer_header->size);
		free(pointer_header);
		return NULL;
	}

	// allocate or reallocate
	struct Memory_Header * reallocated_header = realloc(pointer_header, sizeof(*pointer_header) + size);
	if (reallocated_header != NULL) {
		*reallocated_header = (struct Memory_Header){
			.checksum = (size_t)reallocated_header,
			.size = size,
			.callstack = callstack,
		};

		// track memory
		reallocated_header->next = gs_memory;
		if (gs_memory != NULL) { gs_memory->prev = reallocated_header; }
		gs_memory = reallocated_header;

		return reallocated_header + 1;
	}

	// failed
	struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
	logger_to_console("'realloc' failed: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
	return pointer;
}

void * memory_reallocate_without_tracking(void * pointer, size_t size) {
	struct Callstack const callstack = platform_debug_get_callstack();

	// free
	if (size == 0) { free(pointer); return NULL; }

	// allocate or reallocate
	void * reallocated = realloc(pointer, size);
	if (reallocated != NULL) { return reallocated; }

	// failed
	struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
	logger_to_console("'realloc' failed: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
	return pointer;
}

//
#include "framework/internal/memory_to_system.h"

bool memory_to_system_cleanup(void) {
	if (gs_memory == NULL) { return false; }

	uint32_t const pointer_digits_count = sizeof(size_t) * 2;

	uint32_t total_count = 0;
	uint64_t total_bytes = 0;
	for (struct Memory_Header * it = gs_memory; it; it = it->next) {
		total_count++;
		total_bytes += it->size;
	}

	uint32_t bytes_digits_count = 0;
	for (size_t v = total_bytes; v > 0; v = v / 10) {
		bytes_digits_count++;
	}

	{
		struct CString const header = S_("memory report");
		logger_to_console(
			"> %-*.*s (bytes: %*.zu | count: %u):\n"
			""
			, pointer_digits_count + 4 // compensate for [0x]
			, header.length, header.data
			, bytes_digits_count,  total_bytes
			, total_count
		);
	}

	for (struct Memory_Header const * it = gs_memory; it != NULL; it = it->next) {
		struct CString const stacktrace = platform_debug_get_stacktrace(it->callstack, 1);
		logger_to_console(
			"  [0x%.*zx] (bytes: %*.zu) stacktrace:\n"
			"%.*s"
			""
			, pointer_digits_count, (size_t)(it + 1)
			, bytes_digits_count,   it->size
			, stacktrace.length,    stacktrace.data
		);
	}

	while (gs_memory != NULL) {
		void * pointer = (gs_memory + 1);
		memory_reallocate(pointer, 0);
	}

	return true;
}
