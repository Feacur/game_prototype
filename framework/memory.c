#include "framework/formatter.h"
#include "framework/platform/debug.h"

#include <stdlib.h>

// @todo: custom specialized allocators
// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...
// @idea: use OS-native allocators instead of CRT's

//
#include "memory.h"

// ----- ----- ----- ----- -----
//     generic
// ----- ----- ----- ----- -----

ALLOCATOR(generic_reallocate) {
	struct Memory_Header * header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->checksum != (size_t)header) {
			goto fail;
		}
		header->checksum = 0;
	}

	// free
	if (size == 0) { free(header); return NULL; }

	// allocate or reallocate
	struct Memory_Header * new_header = realloc(header, sizeof(*header) + size);
	if (new_header == NULL) { goto fail; }

	*new_header = (struct Memory_Header){
		.checksum = (size_t)new_header,
		.size = size,
	};

	return new_header + 1;

	// failed
	fail: ERR("'realloc' failed:");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}

// ----- ----- ----- ----- -----
//     debug
// ----- ----- ----- ----- -----

static struct Memory_Header_Debug {
	struct Memory_Header_Debug * prev;
	struct Memory_Header_Debug * next;
	struct Callstack callstack;
	struct Memory_Header base;
} * gs_memory;

ALLOCATOR(debug_reallocate) {
	struct Memory_Header_Debug * header = (pointer != NULL)
		? (struct Memory_Header_Debug *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->base.checksum != ~(size_t)header) {
			goto fail;
		}
		header->base.checksum = 0;

		if (gs_memory == header) { gs_memory = header->next; }
		if (header->next != NULL) { header->next->prev = header->prev; }
		if (header->prev != NULL) { header->prev->next = header->next; }
	}

	// free
	if (size == 0) {
		if (header == NULL) { return NULL; }
		common_memset(header, 0, sizeof(*header) + header->base.size);
		free(header);
		return NULL;
	}

	// allocate or reallocate
	struct Memory_Header_Debug * new_header = realloc(header, sizeof(*header) + size);
	if (new_header == NULL) { goto fail; }

	*new_header = (struct Memory_Header_Debug){
		.base = {
			.checksum = ~(size_t)new_header,
			.size = size,
		},
		.callstack = platform_debug_get_callstack(0),
	};

	new_header->next = gs_memory;
	if (gs_memory != NULL) { gs_memory->prev = new_header; }
	gs_memory = new_header;

	return new_header + 1;

	// failed
	fail: ERR("'realloc' failed:");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}

void memory_debug_report(void) {
	if (gs_memory == NULL) { return; }

	uint32_t const pointer_digits_count = sizeof(size_t) * 2;

	uint32_t total_count = 0;
	uint64_t total_bytes = 0;
	for (struct Memory_Header_Debug * it = gs_memory; it; it = it->next) {
		total_count++;
		total_bytes += it->base.size;
	}

	uint32_t bytes_digits_count = 0;
	for (size_t v = total_bytes; v > 0; v = v / 10) {
		bytes_digits_count++;
	}

	{
		struct CString const header = S_("memory report");
		WRN(
			"> %-*.*s (bytes: %*.zu | count: %u):"
			""
			, pointer_digits_count + 4 // compensate for [0x]
			, header.length, header.data
			, bytes_digits_count,  total_bytes
			, total_count
		);
	}

	for (struct Memory_Header_Debug const * it = gs_memory; it != NULL; it = it->next) {
		struct CString const stacktrace = platform_debug_get_stacktrace(it->callstack);
		WRN(
			"  [0x%.*zx] (bytes: %*.zu) stacktrace:\n"
			"%.*s"
			""
			, pointer_digits_count, (size_t)(it + 1)
			, bytes_digits_count,   it->base.size
			, stacktrace.length,    stacktrace.data
		);
	}

	DEBUG_BREAK();
}

void memory_debug_clear(void) {
	while (gs_memory != NULL) {
		void * const header = gs_memory;
		gs_memory = gs_memory->next;
		free(header);
	}
}
