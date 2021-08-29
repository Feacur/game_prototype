#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

#include <string.h>

//
#include "array_byte.h"

void array_byte_init(struct Array_Byte * array) {
	*array = (struct Array_Byte){0};
}

void array_byte_free(struct Array_Byte * array) {
	MEMORY_FREE(array, array->data);
	memset(array, 0, sizeof(*array));
}

void array_byte_clear(struct Array_Byte * array) {
	array->count = 0;
}

void array_byte_resize(struct Array_Byte * array, uint64_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, target_capacity);
}

static void array_byte_ensure_capacity(struct Array_Byte * array, uint64_t target_count) {
	if (array->capacity < target_count) {
		uint64_t const target_capacity = grow_capacity_value_u64(array->capacity, target_count - array->capacity);
		array_byte_resize(array, target_capacity);
	}
}

void array_byte_push(struct Array_Byte * array, uint8_t value) {
	array_byte_ensure_capacity(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_byte_push_many(struct Array_Byte * array, uint64_t count, uint8_t const * value) {
	array_byte_ensure_capacity(array, array->count + count);
	if (value != NULL) {
		memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_byte_set_many(struct Array_Byte * array, uint32_t index, uint32_t count, uint8_t const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

uint8_t array_byte_pop(struct Array_Byte * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

uint8_t array_byte_peek(struct Array_Byte const * array, uint64_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

uint8_t array_byte_at(struct Array_Byte const * array, uint64_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
