#include "framework/formatter.h"
#include "framework/platform/allocator.h"
#include "framework/platform/debug.h"


//
#include "memory_system.h"

// ----- ----- ----- ----- -----
//     generic
// ----- ----- ----- ----- -----

inline static size_t memory_generic_checksum(void const * pointer) {
	return (size_t)pointer;
}

ALLOCATOR(generic_reallocate) {
	struct Memory_Header * header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->checksum != memory_generic_checksum(header)) {
			ERR("is not generic memory:");
			REPORT_CALLSTACK(); DEBUG_BREAK();
			return NULL;
		}
		header->checksum = 0;
	}

	header = platform_reallocate(header, (size != 0)
		? sizeof(*header) + size
		: 0
	);

	if (header != NULL) {
		*header = (struct Memory_Header){
			.checksum = memory_generic_checksum(header),
			.size = size,
		};

		return header + 1;
	}

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

inline static size_t memory_debug_checksum(void const * pointer) {
	return ~(size_t)pointer;
}

ALLOCATOR(debug_reallocate) {
	struct Memory_Header_Debug * header = (pointer != NULL)
		? (struct Memory_Header_Debug *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->base.checksum != memory_debug_checksum(header)) {
			ERR("> debug memory:");
			PRINT_CALLSTACK(2, header->callstack);
			REPORT_CALLSTACK(); DEBUG_BREAK();
			return NULL;
		}
		header->base.checksum = 0;

		if (gs_memory == header) { gs_memory = header->next; }
		if (header->next != NULL) { header->next->prev = header->prev; }
		if (header->prev != NULL) { header->prev->next = header->next; }
	}

	header = platform_reallocate(header, (size != 0)
		? sizeof(*header) + size
		: 0
	);

	if (header != NULL) {
		*header = (struct Memory_Header_Debug){
			.base = {
				.checksum = memory_debug_checksum(header),
				.size = size,
			},
			.callstack = platform_debug_get_callstack(1),
		};

		header->next = gs_memory;
		if (gs_memory != NULL) { gs_memory->prev = header; }
		gs_memory = header;

		return header + 1;
	}

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
			"> %-*.*s (bytes: %*zu | count: %u):"
			""
			, pointer_digits_count + 4 // compensate for [0x]
			, header.length, header.data
			, bytes_digits_count,  total_bytes
			, total_count
		);
	}

	for (struct Memory_Header_Debug const * it = gs_memory; it != NULL; it = it->next) {
		WRN(
			"  [%#.*zx] (bytes: %*zu) stacktrace:"
			""
			, pointer_digits_count, (size_t)(it + 1)
			, bytes_digits_count,   it->base.size
		);
		PRINT_CALLSTACK(2, it->callstack);
	}

	DEBUG_BREAK();
}

void memory_debug_clear(void) {
	while (gs_memory != NULL) {
		void * const header = gs_memory;
		gs_memory = gs_memory->next;
		platform_reallocate(header, 0);
	}
}
