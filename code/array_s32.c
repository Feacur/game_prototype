#include "memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR) \

//
#include "array_s32.h"

void array_s32_init(struct Array_S32 * array) {
	*array = (struct Array_S32){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_s32_free(struct Array_S32 * array) {
	if (array->capacity == 0 && array->data != NULL) { return; }
	MEMORY_FREE(array->data);
	array_s32_init(array);
}

void array_s32_resize(struct Array_S32 * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_s32_write(struct Array_S32 * array, int32_t value) {
	if (array->count + 1 > array->capacity) {
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_s32_write_many(struct Array_S32 * array, int32_t const * value, uint32_t count) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, count);
	array->count += count;
}
