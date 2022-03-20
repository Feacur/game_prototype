#if !defined(GAME_CONTAINERS_Array_Flt)
#define GAME_CONTAINERS_Array_Flt

#include "framework/common.h"

struct Array_Flt {
	uint32_t capacity, count;
	float * data;
};

void array_flt_init(struct Array_Flt * array);
void array_flt_free(struct Array_Flt * array);

void array_flt_clear(struct Array_Flt * array);
void array_flt_resize(struct Array_Flt * array, uint32_t target_capacity);

void array_flt_push(struct Array_Flt * array, float value);
void array_flt_push_many(struct Array_Flt * array, uint32_t count, float const * value);

void array_flt_set_many(struct Array_Flt * array, uint32_t index, uint32_t count, float const * value);

float array_flt_pop(struct Array_Flt * array);
float array_flt_peek(struct Array_Flt const * array, uint32_t offset);
float array_flt_at(struct Array_Flt const * array, uint32_t index);

#endif
