#include "framework/memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

//
#include "array_pointer.h"

void array_pointer_init(struct Array_Pointer * array) {
	*array = (struct Array_Pointer){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_pointer_free(struct Array_Pointer * array) {
	if (array->capacity == 0) { return; }
	MEMORY_FREE(array->data);
	array_pointer_init(array);
}

void array_pointer_resize(struct Array_Pointer * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_pointer_write(struct Array_Pointer * array, void * value) {
	if (array->count + 1 > array->capacity) {
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_pointer_write_many(struct Array_Pointer * array, uint32_t count, void ** value) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, count * sizeof(*value));
	array->count += count;
}

void array_pointer_write_many_zeroes(struct Array_Pointer * array, uint32_t count) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memset(array->data + array->count, 0, count * sizeof(*array->data));
	array->count += count;
}

#undef GROWTH_FACTOR
#undef GROW_CAPACITY
