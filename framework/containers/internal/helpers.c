#include "framework/maths.h"

#define GROWTH_INITIAL 8
#define GROWTH_LARGE 3
#define GROWTH_SMALL 2

//
#include "helpers.h"

uint32_t growth_adjust_array(uint32_t current, uint32_t target) {
	current = max_u32(current, GROWTH_INITIAL);
	target  = max_u32(target,  GROWTH_INITIAL);

	if (current > mul_div_u32(UINT32_MAX, GROWTH_SMALL, GROWTH_LARGE)) {
		return UINT32_MAX;
	}

	while (current < target) {
		current = mul_div_u32(current, GROWTH_LARGE, GROWTH_SMALL);
	}
	return current;
}

size_t growth_adjust_buffer(size_t current, size_t target) {
	current = max_size(current, GROWTH_INITIAL);
	target  = max_size(target,  GROWTH_INITIAL);

	if (current > mul_div_size(UINT64_MAX, GROWTH_SMALL, GROWTH_LARGE)) {
		return UINT64_MAX;
	}

	while (current < target) {
		current = mul_div_size(current, GROWTH_LARGE, GROWTH_SMALL);
	}
	return current;
}

bool growth_hash_check(uint32_t current, uint32_t target) {
	return target > mul_div_u32(current, GROWTH_SMALL, GROWTH_LARGE);
}

uint32_t growth_hash_adjust(uint32_t current, uint32_t target) {
#if defined(HASH_PO2)
	current = max_u32(current, GROWTH_INITIAL);
	target  = max_u32(target,  GROWTH_INITIAL);
	return po2_next_u32(current + 1);
#else
	return grow_capacity_value_u32(current, target);
#endif
}

#undef GROWTH_INITIAL
#undef GROWTH_LARGE
#undef GROWTH_SMALL
