#include "framework/memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR) \

//
#include "array_byte.h"

void array_byte_init(struct Array_Byte * array) {
	*array = (struct Array_Byte){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_byte_free(struct Array_Byte * array) {
	if (array->capacity == 0) { return; }
	MEMORY_FREE(array->data);
	array_byte_init(array);
}

void array_byte_resize(struct Array_Byte * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_byte_write(struct Array_Byte * array, uint8_t value) {
	if (array->count + 1 > array->capacity) {
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_byte_write_many(struct Array_Byte * array, uint32_t count, uint8_t const * value) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, count * sizeof(*value));
	array->count += count;
}

void array_byte_write_many_zeroes(struct Array_Byte * array, uint32_t count) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memset(array->data + array->count, 0, count * sizeof(*array->data));
	array->count += count;
}
