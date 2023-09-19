//
#include "strings.h"

struct Strings strings_init(void) {
	return (struct Strings){
		.offsets = array_init(sizeof(uint32_t)),
		.lengths = array_init(sizeof(uint32_t)),
		.buffer = buffer_init(NULL),
	};
}

void strings_free(struct Strings * strings) {
	array_free(&strings->offsets);
	array_free(&strings->lengths);
	buffer_free(&strings->buffer);
	// common_memset(strings, 0, sizeof(*strings));
}

uint32_t strings_find(struct Strings const * strings, struct CString value) {
	if (value.length == 0) { return 0; }
	if (value.data == NULL) { return 0; }
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		uint32_t const * length_at = array_at(&strings->lengths, i);
		if (value.length == *length_at) {
			if (common_memcmp(value.data, (uint8_t *)strings->buffer.data + offset, value.length) == 0) { return i + 1; }
		}
		offset += *length_at + 1;
	}
	return 0;
}

uint32_t strings_add(struct Strings * strings, struct CString value) {
	if (value.length == 0) { return 0; }
	if (value.data == NULL) { return 0; }
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		uint32_t const * length_at = array_at(&strings->lengths, i);
		if (value.length == *length_at) {
			if (common_memcmp(value.data, (uint8_t *)strings->buffer.data + offset, value.length) == 0) { return i + 1; }
		}
		offset += *length_at + 1;
	}

	array_push_many(&strings->offsets, 1, &offset);
	array_push_many(&strings->lengths, 1, &value.length);
	buffer_push_many(&strings->buffer, value.length, value.data);
	buffer_push_many(&strings->buffer, 1, "\0");

	return strings->lengths.count;
}

struct CString strings_get(struct Strings const * strings, uint32_t id) {
	if (id > strings->lengths.count) { return (struct CString){0}; }
	uint32_t const index = id - 1;
	uint32_t const * length_at = array_at(&strings->lengths, index);
	uint32_t const * offset_at = array_at(&strings->offsets, index);
	return (struct CString){
		.length = *length_at,
		.data = (char const *)strings->buffer.data + *offset_at,
	};
}
