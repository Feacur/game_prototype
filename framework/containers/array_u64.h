#if !defined(GAME_CONTAINERS_ARRAY_U64)
#define GAME_CONTAINERS_ARRAY_U64

#include "framework/common.h"

struct Array_U64 {
	uint32_t capacity, count;
	uint64_t * data;
};

struct Array_U64 array_u64_init(void);
void array_u64_free(struct Array_U64 * array);

void array_u64_clear(struct Array_U64 * array);
void array_u64_resize(struct Array_U64 * array, uint32_t target_capacity);
void array_u64_ensure(struct Array_U64 * array, uint32_t target_capacity);

void array_u64_push(struct Array_U64 * array, uint64_t value);
void array_u64_push_many(struct Array_U64 * array, uint32_t count, uint64_t const * value);

void array_u64_set_many(struct Array_U64 * array, uint32_t index, uint32_t count, uint64_t const * value);

uint64_t array_u64_pop(struct Array_U64 * array);
uint64_t array_u64_peek(struct Array_U64 const * array, uint32_t offset);
uint64_t array_u64_at(struct Array_U64 const * array, uint32_t index);

#endif
