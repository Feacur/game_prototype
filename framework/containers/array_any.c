#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

#include <string.h>

//
#include "array_any.h"

void array_any_init(struct Array_Any * array, uint32_t value_size) {
	*array = (struct Array_Any){
		.value_size = value_size,
	};
}

void array_any_free(struct Array_Any * array) {
	MEMORY_FREE(array, array->data);
	memset(array, 0, sizeof(*array));
}

void array_any_clear(struct Array_Any * array) {
	array->count = 0;
}

void array_any_resize(struct Array_Any * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->value_size * target_capacity);
}

static void array_any_ensure_capacity(struct Array_Any * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_any_resize(array, target_capacity);
	}
}

void array_any_push(struct Array_Any * array, void const * value) {
	array_any_ensure_capacity(array, array->count + 1);
	memcpy(
		array->data + array->value_size * array->count,
		value,
		array->value_size
	);
	array->count++;
}

void array_any_push_many(struct Array_Any * array, uint32_t count, void const * value) {
	array_any_ensure_capacity(array, array->count + count);
	if (value != NULL) {
		memcpy(
			array->data + array->value_size * array->count,
			value,
			array->value_size * count
		);
	}
	array->count += count;
}

void array_any_set_many(struct Array_Any * array, uint32_t index, uint32_t count, void const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	memcpy(
		array->data + array->value_size * index,
		value,
		array->value_size * count
	);
}

void * array_any_pop(struct Array_Any * array) {
	if (array->count == 0) { return NULL; }
	array->count--;
	return array->data + array->value_size * array->count;
}

void * array_any_peek(struct Array_Any const * array, uint32_t offset) {
	if (offset >= array->count) { return NULL; }
	return array->data + array->value_size * (array->count - offset - 1);
}

void * array_any_at(struct Array_Any const * array, uint32_t index) {
	if (index >= array->count) { return NULL; }
	return array->data + array->value_size * index;
}
