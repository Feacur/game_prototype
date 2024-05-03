#include "framework/containers/array.h"
#include "framework/containers/buffer.h"

//
#include "strings.h"

static struct Strings {
	struct Array offsets; // `uint32_t`
	struct Array lengths; // `uint32_t`
	struct Buffer buffer; // null-terminated chunks
	struct Handle h_free; // tracks generations
} gs_strings = {
	.offsets = {
		.value_size = sizeof(uint32_t),
	},
	.lengths = {
		.value_size = sizeof(uint32_t),
	},
};

void system_strings_clear(bool deallocate) {
	array_clear(&gs_strings.offsets, deallocate);
	array_clear(&gs_strings.lengths, deallocate);
	buffer_clear(&gs_strings.buffer, deallocate);
	gs_strings.h_free.gen++;
}

struct Handle system_strings_add(struct CString value) {
	struct Handle handle = system_strings_find(value);
	if (!handle_is_null(handle)) { return handle; }

	uint32_t const offset = (uint32_t)gs_strings.buffer.size;
	array_push_many(&gs_strings.offsets, 1, &offset);
	array_push_many(&gs_strings.lengths, 1, &value.length);
	buffer_push_many(&gs_strings.buffer, value.length, value.data);
	buffer_push_many(&gs_strings.buffer, 1, "\0");

	uint32_t const id = gs_strings.lengths.count;
	return (struct Handle){
		.id = id,
		.gen = gs_strings.h_free.gen,
	};
}

struct Handle system_strings_find(struct CString value) {
	if (cstring_empty(value)) { return (struct Handle){0}; }
	uint32_t offset = 0;
	FOR_ARRAY(&gs_strings.lengths, it) {
		struct CString const word = {
			.length = *(uint32_t *)it.value,
			.data = buffer_at(&gs_strings.buffer, offset),
		};
		if (cstring_equals(word, value)) {
			return (struct Handle){
				.id = it.curr + 1,
				.gen = gs_strings.h_free.gen,
			};
		}
		offset += word.length + 1;
	}
	return (struct Handle){0};
}

struct CString system_strings_get(struct Handle handle) {
	if (handle_is_null(handle))               { return (struct CString){0}; }
	if (handle.id > gs_strings.lengths.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return (struct CString){0}; }
	if (handle.gen != gs_strings.h_free.gen)  { REPORT_CALLSTACK(); DEBUG_BREAK(); return (struct CString){0}; }

	uint32_t const id = handle.id - 1;
	uint32_t const * offset_at = array_at_unsafe(&gs_strings.offsets, id);
	return (struct CString){
		.length = *(uint32_t *)array_at_unsafe(&gs_strings.lengths, id),
		.data = buffer_at_unsafe(&gs_strings.buffer, *offset_at),
	};
}
