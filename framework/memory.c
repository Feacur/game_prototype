#include "framework/formatter.h"
#include "framework/platform_debug.h"

#include <stdlib.h>

// @todo: custom specialized allocators
// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...
// @idea: use OS-native allocators instead of CRT's

//
#include "memory.h"

static struct Memory_Header {
	size_t checksum, size;
	struct Memory_Header * prev;
	struct Memory_Header * next;
	struct Callstack callstack;
} * gs_memory;

ALLOCATOR(memory_reallocate) {
	struct Memory_Header * pointer_header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (pointer_header != NULL) {
		if (pointer_header->checksum != (size_t)pointer_header) {
			goto fail;
		}

		if (gs_memory == pointer_header) { gs_memory = pointer_header->next; }
		if (pointer_header->next != NULL) { pointer_header->next->prev = pointer_header->prev; }
		if (pointer_header->prev != NULL) { pointer_header->prev->next = pointer_header->next; }
		pointer_header->checksum = 0;
		// common_memset(pointer_header, 0, sizeof(*pointer_header) + pointer_header->size);
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
	if (reallocated_header == NULL) { goto fail; }

	// track memory
	*reallocated_header = (struct Memory_Header){
		.checksum = (size_t)reallocated_header,
		.size = size,
		.callstack = platform_debug_get_callstack(0),
	};

	reallocated_header->next = gs_memory;
	if (gs_memory != NULL) { gs_memory->prev = reallocated_header; }
	gs_memory = reallocated_header;

	return reallocated_header + 1;

	// failed
	fail: LOG("'realloc' failed:\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}

ALLOCATOR(memory_reallocate_without_tracking) {
	// free
	if (size == 0) { free(pointer); return NULL; }

	// allocate or reallocate
	void * reallocated = realloc(pointer, size);
	if (reallocated == NULL) { goto fail; }

	return reallocated;

	// failed
	fail: LOG("'realloc' failed:\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}

//
#include "framework/internal/memory_to_system.h"

bool memory_to_system_init(void) { return true; }

void memory_to_system_free(void) {
	if (gs_memory == NULL) { return; }

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
		LOG(
			"> %-*.*s (bytes: %*.zu | count: %u):\n"
			""
			, pointer_digits_count + 4 // compensate for [0x]
			, header.length, header.data
			, bytes_digits_count,  total_bytes
			, total_count
		);
	}

	for (struct Memory_Header const * it = gs_memory; it != NULL; it = it->next) {
		struct CString const stacktrace = platform_debug_get_stacktrace(it->callstack);
		LOG(
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

	DEBUG_BREAK();
}
