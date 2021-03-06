#if !defined(GAME_CONTAINERS_ARRAY_U32)
#define GAME_CONTAINERS_ARRAY_U32

#include "framework/common.h"

struct Array_U32 {
	uint32_t capacity, count;
	uint32_t * data;
};

void array_u32_init(struct Array_U32 * array);
void array_u32_free(struct Array_U32 * array);

void array_u32_clear(struct Array_U32 * array);
void array_u32_resize(struct Array_U32 * array, uint32_t target_capacity);

void array_u32_push(struct Array_U32 * array, uint32_t value);
void array_u32_push_many(struct Array_U32 * array, uint32_t count, uint32_t const * value);

uint32_t array_u32_pop(struct Array_U32 * array);
uint32_t array_u32_peek(struct Array_U32 * array, uint32_t offset);
uint32_t array_u32_at(struct Array_U32 * array, uint32_t index);

#endif
