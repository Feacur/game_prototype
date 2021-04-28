#if !defined(GAME_CONTAINTERS_ARRAY_U64)
#define GAME_CONTAINTERS_ARRAY_U64

#include "framework/common.h"

struct Array_U64 {
	uint32_t capacity, count;
	uint64_t * data;
};

void array_u64_init(struct Array_U64 * array);
void array_u64_free(struct Array_U64 * array);

void array_u64_resize(struct Array_U64 * array, uint32_t size);
void array_u64_write(struct Array_U64 * array, uint32_t value);
void array_u64_write_many(struct Array_U64 * array, uint32_t count, uint32_t const * value);
void array_u64_write_many_zeroes(struct Array_U64 * array, uint32_t count);

#endif
