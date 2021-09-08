//
#include "strings.h"

void strings_init(struct Strings * strings) {
	array_u32_init(&strings->offsets);
	array_u32_init(&strings->lengths);
	array_byte_init(&strings->buffer);
}

void strings_free(struct Strings * strings) {
	array_u32_free(&strings->offsets);
	array_u32_free(&strings->lengths);
	array_byte_free(&strings->buffer);
}

uint32_t strings_find(struct Strings const * strings, struct CString value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (value.length == strings->lengths.data[i]) {
			if (common_memcmp(value.data, strings->buffer.data + offset, value.length) == 0) { return i; }
		}
		offset += strings->lengths.data[i] + 1;
	}
	return INDEX_EMPTY;
}

uint32_t strings_add(struct Strings * strings, struct CString value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (value.length == strings->lengths.data[i]) {
			if (common_memcmp(value.data, strings->buffer.data + offset, value.length) == 0) { return i; }
		}
		offset += strings->lengths.data[i] + 1;
	}

	array_u32_push(&strings->offsets, offset);
	array_u32_push(&strings->lengths, value.length);
	array_byte_push_many(&strings->buffer, value.length, (uint8_t const *)value.data);
	array_byte_push(&strings->buffer, '\0');

	return strings->lengths.count - 1;
}

struct CString strings_get(struct Strings const * strings, uint32_t id) {
	if (id >= strings->lengths.count) { return S_NULL; }
	return (struct CString){
		.length = strings->lengths.data[id],
		.data = (char const *)(strings->buffer.data + strings->offsets.data[id]),
	};
}
