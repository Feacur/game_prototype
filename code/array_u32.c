#include "memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR) \

//
#include "array_u32.h"

void array_u32_init(struct Array_U32 * array) {
	*array = (struct Array_U32){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_u32_free(struct Array_U32 * array) {
	if (array->capacity == 0 && array->data != NULL) { return; }
	MEMORY_FREE(array->data);
	array_u32_init(array);
}

void array_u32_resize(struct Array_U32 * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_u32_write(struct Array_U32 * array, uint32_t value) {
	if (array->count + 1 > array->capacity) {
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_u32_write_many(struct Array_U32 * array, uint32_t count, uint32_t const * value) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, count * sizeof(*value));
	array->count += count;
}
