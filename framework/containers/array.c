#include "framework/maths.h"
#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

//
#include "array.h"

struct Array array_init(uint32_t value_size) {
	return (struct Array){
		.value_size = value_size,
	};
}

void array_free(struct Array * array) {
	MEMORY_FREE(array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_clear(struct Array * array) {
	array->count = 0;
}

void array_resize(struct Array * array, uint32_t capacity) {
	if (array->capacity == capacity) { return; }
	void * data = memory_reallocate(array->data, array->value_size * capacity);
	if (data == NULL) { return; }
	array->capacity = capacity;
	array->count = min_u32(array->count, capacity);
	array->data = data;
}

void array_ensure(struct Array * array, uint32_t capacity) {
	if (array->capacity >= capacity) { return; }
	capacity = growth_adjust_array(array->capacity, capacity);
	array_resize(array, capacity);
}

void array_push_many(struct Array * array, uint32_t count, void const * value) {
	array_ensure(array, array->count + count);
	uint8_t * end = (uint8_t *)array->data + array->value_size * array->count;
	common_memcpy(end, value, array->value_size * count);
	array->count += count;
}

void array_set_many(struct Array * array, uint32_t index, uint32_t count, void const * value) {
	if (index + count > array->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	uint8_t * at = (uint8_t *)array->data + array->value_size * index;
	common_memcpy(at, value, array->value_size * count);
}

void array_insert_many(struct Array * array, uint32_t index, uint32_t count, void const * value) {
	if (index > array->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	array_ensure(array, array->count + count);
	//
	size_t const size = array->value_size * count;
	uint8_t * at  = (uint8_t *)array->data + array->value_size * index;
	uint8_t * end = (uint8_t *)array->data + array->value_size * array->count;
	for (uint8_t * it = end; it > at; it -= array->value_size) {
		common_memcpy(it, it - size, array->value_size);
	}
	common_memcpy(at, value, size);
	array->count += count;
}

void * array_pop(struct Array * array, uint32_t count) {
	if (array->count < count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	array->count -= count;
	size_t const offset = array->value_size * array->count;
	return (uint8_t *)array->data + offset;
}

void * array_peek(struct Array const * array, uint32_t depth) {
	if (depth >= array->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = array->value_size * (array->count - depth - 1);
	return (uint8_t *)array->data + offset;
}

void * array_at(struct Array const * array, uint32_t index) {
	if (index >= array->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = array->value_size * index;
	return (uint8_t *)array->data + offset;
}

bool array_iterate(struct Array const * array, struct Array_Iterator * iterator) {
	while (iterator->next < array->count) {
		uint32_t const index = iterator->next++;
		iterator->curr = index;
		iterator->value = array_at(array, index);
		return true;
	}
	return false;
}
