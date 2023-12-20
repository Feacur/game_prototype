#include "framework/containers/array.h"
#include "framework/containers/buffer.h"

//
#include "string_system.h"

static struct String_System {
	struct Array offsets; // uint32_t
	struct Array lengths; // uint32_t
	struct Buffer buffer; // null-terminated chunks
} gs_string_system = {
	.offsets = {
		.value_size = sizeof(uint32_t),
	},
	.lengths = {
		.value_size = sizeof(uint32_t),
	},
};

void string_system_clear(bool deallocate) {
	array_clear(&gs_string_system.offsets, deallocate);
	array_clear(&gs_string_system.lengths, deallocate);
	buffer_clear(&gs_string_system.buffer, deallocate);
}

uint32_t string_system_add(struct CString value) {
	uint32_t id = string_system_find(value);
	if (id != 0) { return id; }

	uint32_t const offset = (uint32_t)gs_string_system.buffer.size;
	array_push_many(&gs_string_system.offsets, 1, &offset);
	array_push_many(&gs_string_system.lengths, 1, &value.length);
	buffer_push_many(&gs_string_system.buffer, value.length, value.data);
	buffer_push_many(&gs_string_system.buffer, 1, "\0");

	return gs_string_system.lengths.count;
}

uint32_t string_system_find(struct CString value) {
	if (cstring_empty(value)) { return 0; }
	uint32_t offset = 0;
	FOR_ARRAY(&gs_string_system.lengths, it) {
		struct CString const word = {
			.length = *(uint32_t *)it.value,
			.data = buffer_at(&gs_string_system.buffer, offset),
		};
		if (cstring_equals(word, value)) { return it.curr + 1; }
		offset += word.length + 1;
	}
	return 0;
}

struct CString string_system_get(uint32_t id) {
	if (id > gs_string_system.lengths.count) { return (struct CString){0}; }
	uint32_t const index = id - 1;
	uint32_t const * offset_at = array_at_unsafe(&gs_string_system.offsets, index);
	return (struct CString){
		.length = *(uint32_t *)array_at_unsafe(&gs_string_system.lengths, index),
		.data = buffer_at_unsafe(&gs_string_system.buffer, *offset_at),
	};
}
