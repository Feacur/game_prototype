#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "array_any.h"

struct Array_Any array_any_init(uint32_t value_size) {
	return (struct Array_Any){
		.value_size = value_size,
	};
}

void array_any_free(struct Array_Any * array) {
	MEMORY_FREE(array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_any_clear(struct Array_Any * array) {
	array->count = 0;
}

void array_any_resize(struct Array_Any * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = memory_reallocate(array->data, array->value_size * target_capacity);
}

void array_any_ensure(struct Array_Any * array, uint32_t target_capacity) {
	if (array->capacity < target_capacity) {
		array_any_resize(array, target_capacity);
	}
}

static void array_any_grow_if_must(struct Array_Any * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_any_resize(array, target_capacity);
	}
}

void array_any_push_many(struct Array_Any * array, uint32_t count, void const * value) {
	array_any_grow_if_must(array, array->count + count);
	//
	uint32_t const size = array->value_size * count;
	uint8_t * end = (uint8_t *)array->data + array->value_size * array->count;
	common_memcpy(end, value, size);
	array->count += count;
}

void array_any_set_many(struct Array_Any * array, uint32_t index, uint32_t count, void const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds\n"); DEBUG_BREAK(); return; }
	uint32_t const size = array->value_size * count;
	uint8_t * at = (uint8_t *)array->data + array->value_size * index;
	common_memcpy(at, value, size);
}

void array_any_insert_many(struct Array_Any * array, uint32_t index, uint32_t count, void const * value) {
	if (index > array->count) { logger_to_console("out of bounds\n"); DEBUG_BREAK(); return; }
	array_any_grow_if_must(array, array->count + count);
	//
	uint32_t const size = array->value_size * count;
	uint8_t * end = (uint8_t *)array->data + array->value_size * array->count;
	uint8_t * at  = (uint8_t *)array->data + array->value_size * index;
	for (uint8_t * it = end; it > at; it -= array->value_size) {
		common_memcpy(it, it - size, array->value_size);
	}
	common_memcpy(at, value, size);
	array->count += count;
}

void * array_any_pop(struct Array_Any * array) {
	if (array->count == 0) { return NULL; }
	array->count--;
	return (uint8_t *)array->data + array->value_size * array->count;
}

void * array_any_peek(struct Array_Any const * array, uint32_t offset) {
	if (offset >= array->count) { return NULL; }
	return (uint8_t *)array->data + array->value_size * (array->count - offset - 1);
}

void * array_any_at(struct Array_Any const * array, uint32_t index) {
	if (index >= array->count) { return NULL; }
	return (uint8_t *)array->data + array->value_size * index;
}
