#if !defined(GAME_ARRAY_S32)
#define GAME_ARRAY_S32

#include "common.h"

struct Array_S32 {
	uint32_t capacity, count;
	int32_t * data;
};

void array_s32_init(struct Array_S32 * array);
void array_s32_free(struct Array_S32 * array);

void array_s32_resize(struct Array_S32 * array, uint32_t size);
void array_s32_write(struct Array_S32 * array, int32_t value);
void array_s32_write_many(struct Array_S32 * array, int32_t const * value, uint32_t count);

#endif
