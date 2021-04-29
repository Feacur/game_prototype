#if !defined(GAME_CONTAINERS_ARRAY_S32)
#define GAME_CONTAINERS_ARRAY_S32

#include "framework/common.h"

struct Array_S32 {
	uint32_t capacity, count;
	int32_t * data;
};

void array_s32_init(struct Array_S32 * array);
void array_s32_free(struct Array_S32 * array);

void array_s32_clear(struct Array_S32 * array);
void array_s32_resize(struct Array_S32 * array, uint32_t target_capacity);

void array_s32_push(struct Array_S32 * array, int32_t value);
void array_s32_push_many(struct Array_S32 * array, uint32_t count, int32_t const * value);

int32_t array_s32_pop(struct Array_S32 * array);
int32_t array_s32_peek(struct Array_S32 * array, uint32_t offset);
int32_t array_s32_at(struct Array_S32 * array, uint32_t index);

#endif
