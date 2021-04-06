#include "framework/containers/array_byte.h"
#include "framework/containers/array_u32.h"

#include "framework/memory.h"

#include <string.h>

//
#include "strings.h"

struct Strings {
	struct Array_U32 offsets;
	struct Array_U32 lengths;
	struct Array_Byte buffer;
};

struct Strings * strings_init(void) {
	struct Strings * strings = MEMORY_ALLOCATE(NULL, struct Strings);
	array_u32_init(&strings->offsets);
	array_u32_init(&strings->lengths);
	array_byte_init(&strings->buffer);
	return strings;
}

void strings_free(struct Strings * strings) {
	array_u32_free(&strings->offsets);
	array_u32_free(&strings->lengths);
	array_byte_free(&strings->buffer);
	MEMORY_FREE(strings, strings);
}

uint32_t strings_find(struct Strings * strings, uint32_t length, char const * value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (length == strings->lengths.data[i]) {
			if (memcmp(value, strings->buffer.data + offset, length) == 0) { return i; }
		}
		offset += strings->lengths.data[i] + 1;
	}
	return 0;
}

uint32_t strings_add(struct Strings * strings, uint32_t length, char const * value) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < strings->lengths.count; i++) {
		if (length == strings->lengths.data[i]) {
			if (memcmp(value, strings->buffer.data + offset, length) == 0) { return i; }
		}
		offset += strings->lengths.data[i] + 1;
	}

	array_u32_write(&strings->offsets, offset);
	array_u32_write(&strings->lengths, length);
	array_byte_write_many(&strings->buffer, length, (uint8_t const *)value);
	array_byte_write(&strings->buffer, '\0');

	return strings->lengths.count - 1;
}

char const * strings_get(struct Strings * strings, uint32_t id) {
	if (id >= strings->lengths.count) { return NULL; }
	return (char const *)strings->buffer.data + strings->offsets.data[id];
}
