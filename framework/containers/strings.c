//
#include "strings.h"

struct Strings strings_init(void) {
	return (struct Strings){
		.offsets = array_u32_init(),
		.lengths = array_u32_init(),
		.buffer = buffer_init(),
	};
}

void strings_free(struct Strings * strings) {
	array_u32_free(&strings->offsets);
	array_u32_free(&strings->lengths);
	buffer_free(&strings->buffer);
}

uint32_t strings_find(struct Strings const * strings, struct CString value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (value.length == strings->lengths.data[i]) {
			if (common_memcmp(value.data, strings->buffer.data + offset, value.length) == 0) { return i + 1; }
		}
		offset += strings->lengths.data[i] + 1;
	}
	return c_string_id_empty;
}

uint32_t strings_add(struct Strings * strings, struct CString value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (value.length == strings->lengths.data[i]) {
			if (common_memcmp(value.data, strings->buffer.data + offset, value.length) == 0) { return i + 1; }
		}
		offset += strings->lengths.data[i] + 1;
	}

	array_u32_push(&strings->offsets, offset);
	array_u32_push(&strings->lengths, value.length);
	buffer_push_many(&strings->buffer, value.length, (uint8_t const *)value.data);
	buffer_push(&strings->buffer, '\0');

	return strings->lengths.count;
}

struct CString strings_get(struct Strings const * strings, uint32_t id) {
	if (id > strings->lengths.count) { return S_NULL; }
	uint32_t const index = id - 1;
	return (struct CString){
		.length = strings->lengths.data[index],
		.data = (char const *)(strings->buffer.data + strings->offsets.data[index]),
	};
}

//

uint32_t const c_string_id_empty = 0;
