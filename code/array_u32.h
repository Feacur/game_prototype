#if !defined(GAME_ARRAY_U32)
#define GAME_ARRAY_U32

#include "common.h"

struct Array_U32 {
	uint32_t capacity, count;
	uint32_t * data;
};

void array_u32_init(struct Array_U32 * array);
void array_u32_free(struct Array_U32 * array);

void array_u32_resize(struct Array_U32 * array, uint32_t size);
void array_u32_write(struct Array_U32 * array, uint32_t value);
void array_u32_write_many(struct Array_U32 * array, uint32_t const * value, uint32_t count);

#endif
