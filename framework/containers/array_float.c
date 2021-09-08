#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "array_float.h"

void array_float_init(struct Array_Float * array) {
	*array = (struct Array_Float){0};
}

void array_float_free(struct Array_Float * array) {
	MEMORY_FREE(array, array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_float_clear(struct Array_Float * array) {
	array->count = 0;
}

void array_float_resize(struct Array_Float * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, target_capacity);
}

static void array_float_ensure_capacity(struct Array_Float * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_float_resize(array, target_capacity);
	}
}

void array_float_push(struct Array_Float * array, float value) {
	array_float_ensure_capacity(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_float_push_many(struct Array_Float * array, uint32_t count, float const * value) {
	array_float_ensure_capacity(array, array->count + count);
	if (value != NULL) {
		common_memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_float_set_many(struct Array_Float * array, uint32_t index, uint32_t count, float const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	common_memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

float array_float_pop(struct Array_Float * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

float array_float_peek(struct Array_Float const * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

float array_float_at(struct Array_Float const * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
