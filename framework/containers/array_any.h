#if !defined(GAME_CONTAINERS_ARRAY_ANY)
#define GAME_CONTAINERS_ARRAY_ANY

#include "framework/common.h"

struct Array_Any {
	uint32_t value_size;
	uint32_t capacity, count;
	uint8_t * data;
};

void array_any_init(struct Array_Any * array, uint32_t value_size);
void array_any_free(struct Array_Any * array);

void array_any_clear(struct Array_Any * array);
void array_any_resize(struct Array_Any * array, uint32_t target_capacity);

void array_any_push(struct Array_Any * array, void const * value);
void array_any_push_many(struct Array_Any * array, uint32_t count, void const * value);

void array_any_set_many(struct Array_Any * array, uint32_t index, uint32_t count, void const * value);

void * array_any_pop(struct Array_Any * array);
void * array_any_peek(struct Array_Any * array, uint32_t offset);
void * array_any_at(struct Array_Any * array, uint32_t index);

#endif
