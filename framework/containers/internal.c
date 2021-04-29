#include "framework/maths.h"

#include <stdio.h>

//
#include "internal.h"

uint32_t grow_capacity_value_u32(uint32_t current, uint32_t delta) {
	uint32_t target = current + delta;

	#if defined(GAME_TARGET_DEBUG)
	if (target < current) {
		fprintf(stderr, "requested value is too large\n"); DEBUG_BREAK();
		return current;
	}
	#endif

	if (target < 8) { return 8; }
	while (current < target) {
		current = mul_div_u32(current, 3, 2);
	}

	#if defined(GAME_TARGET_DEBUG)
	if (current < target) {
		fprintf(stderr, "requested value is too large\n"); DEBUG_BREAK();
		return target;
	}
	#endif

	return current;
}

uint64_t grow_capacity_value_u64(uint64_t current, uint64_t delta) {
	uint64_t target = current + delta;

	#if defined(GAME_TARGET_DEBUG)
	if (target < current) {
		fprintf(stderr, "requested value is too large\n"); DEBUG_BREAK();
		return current;
	}
	#endif

	if (target < 8) { return 8; }
	while (current < target) {
		current = mul_div_u64(current, 3, 2);
	}

	#if defined(GAME_TARGET_DEBUG)
	if (current < target) {
		fprintf(stderr, "requested value is too large\n"); DEBUG_BREAK();
		return target;
	}
	#endif

	return current;
}

bool should_hash_table_grow(uint32_t capacity, uint32_t count) {
	return count >= mul_div_u64(capacity, 2, 3);
}

uint32_t adjust_hash_table_capacity_value(uint32_t value) {
	#if defined(GAME_TARGET_DEBUG)
	if (value > 0x80000000) {
		fprintf(stderr, "requested value is too large\n"); DEBUG_BREAK();
		return 0x80000000;
	}
	#endif

	return round_up_to_PO2_u32(value);
}
