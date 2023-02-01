#if !defined(FRAMEWORK_CONTAINERS_ARRAY_FLT)
#define FRAMEWORK_CONTAINERS_ARRAY_FLT

#include "framework/common.h"

struct Array_Flt {
	uint32_t capacity, count;
	float * data;
};

struct Array_Flt array_flt_init(void);
void array_flt_free(struct Array_Flt * array);

void array_flt_clear(struct Array_Flt * array);
void array_flt_resize(struct Array_Flt * array, uint32_t target_capacity);
void array_flt_ensure(struct Array_Flt * array, uint32_t target_capacity);

void array_flt_push_many(struct Array_Flt * array, uint32_t count, float const * value);
void array_flt_set_many(struct Array_Flt * array, uint32_t index, uint32_t count, float const * value);
void array_flt_insert_many(struct Array_Flt * array, uint32_t index, uint32_t count, float const * value);

float array_flt_pop(struct Array_Flt * array);
float array_flt_peek(struct Array_Flt const * array, uint32_t offset);
float array_flt_at(struct Array_Flt const * array, uint32_t index);

#endif
