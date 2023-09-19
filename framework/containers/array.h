#if !defined(FRAMEWORK_CONTAINERS_ARRAY)
#define FRAMEWORK_CONTAINERS_ARRAY

#include "framework/common.h"

struct Array {
	uint32_t value_size;
	uint32_t capacity, count;
	void * data;
};

struct Array array_init(uint32_t value_size);
void array_free(struct Array * array);

void array_clear(struct Array * array);
void array_resize(struct Array * array, uint32_t target_capacity);
void array_ensure(struct Array * array, uint32_t target_capacity);

void array_push_many(struct Array * array, uint32_t count, void const * value);
void array_set_many(struct Array * array, uint32_t index, uint32_t count, void const * value);
void array_insert_many(struct Array * array, uint32_t index, uint32_t count, void const * value);

void * array_pop(struct Array * array);
void * array_peek(struct Array const * array, uint32_t offset);
void * array_at(struct Array const * array, uint32_t index);

#endif
