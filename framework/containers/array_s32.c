#include "framework/memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

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
	if (array->capacity == 0) { return; }
	MEMORY_FREE(array, array->data);
	array_s32_init(array);
}

void array_s32_resize(struct Array_S32 * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_s32_write(struct Array_S32 * array, int32_t value) {
	if (array->count + 1 > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_s32_write_many(struct Array_S32 * array, uint32_t count, int32_t const * value) {
	if (array->count + count > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, sizeof(*value) * count);
	array->count += count;
}

void array_s32_write_many_zeroes(struct Array_S32 * array, uint32_t count) {
	if (array->count + count > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	memset(array->data + array->count, 0, sizeof(*array->data) * count);
	array->count += count;
}

#undef GROWTH_FACTOR
#undef GROW_CAPACITY
