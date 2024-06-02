#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/systems/memory.h"

#include "internal/helpers.h"


//
#include "array.h"

struct Array array_init(uint32_t value_size) {
	return (struct Array){
		.value_size = value_size,
	};
}

void array_free(struct Array * array) {
	if (array->allocate == NULL) { return; }
	array->allocate(array->data, 0);
	cbuffer_clear(CBMP_(array));
}

void array_clear(struct Array * array) {
	array->count = 0;
}

void array_resize(struct Array * array, uint32_t capacity) {
	if (array->capacity == capacity) { return; }
	if (array->allocate == NULL) {
		array->allocate = DEFAULT_REALLOCATOR;
	}
	void * data = array->allocate(array->data, capacity * array->value_size);
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

uint32_t array_find_if(struct Array * array, Predicate * predicate) {
	FOR_ARRAY(array, it) {
		if (predicate(it.value)) {
			return it.curr;
		}
	}
	return INDEX_EMPTY;
}

void array_remove_if(struct Array * array, Predicate * predicate) {
	uint32_t empty_index = array_find_if(array, predicate);
	if (empty_index == INDEX_EMPTY) { return; }
	for (uint32_t i = empty_index + 1; i < array->count; i++) {
		void const * value = array_at_unsafe(array, i);
		if (predicate(value)) { continue; }

		void * empty = array_at_unsafe(array, empty_index++);
		common_memcpy(empty, value, array->value_size);
	}
	array->count = empty_index;
}

void array_push_many(struct Array * array, uint32_t count, void const * value) {
	array_ensure(array, array->count + count);
	uint8_t * end = array_at_unsafe(array, array->count);
	common_memcpy(end, value, array->value_size * count);
	array->count += count;
}

void array_set_many(struct Array * array, uint32_t index, uint32_t count, void const * value) {
	if (index + count > array->count) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	uint8_t * at = array_at_unsafe(array, index);
	common_memcpy(at, value, array->value_size * count);
}

void array_insert_many(struct Array * array, uint32_t index, uint32_t count, void const * value) {
	if (index > array->count) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	array_ensure(array, array->count + count);
	//
	size_t const size = array->value_size * count;
	uint8_t * at  = array_at_unsafe(array, index);
	uint8_t * end = array_at_unsafe(array, array->count);
	for (uint8_t * it = end; it > at; it -= array->value_size) {
		common_memcpy(it, it - size, array->value_size);
	}
	common_memcpy(at, value, size);
	array->count += count;
}

void * array_pop(struct Array * array, uint32_t count) {
	if (array->count < count) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	array->count -= count;
	return array_at_unsafe(array, array->count);
}

void * array_peek(struct Array const * array, uint32_t depth) {
	if (depth >= array->count) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	return array_at_unsafe(array, array->count - depth - 1);
}

void * array_at(struct Array const * array, uint32_t index) {
	if (index >= array->count) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	return array_at_unsafe(array, index);
}

void * array_at_unsafe(struct Array const * array, uint32_t index) {
	// @note: places that call it shoud do the check
	// if (index >= array->capacity) { return NULL; }
	size_t const offset = array->value_size * index;
	return (uint8_t *)array->data + offset;
}

bool array_iterate(struct Array const * array, struct Array_Iterator * iterator) {
	while (iterator->next < array->count) {
		uint32_t const index = iterator->next++;
		iterator->curr  = index;
		iterator->value = array_at_unsafe(array, index);
		return true;
	}
	return false;
}
