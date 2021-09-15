#include "framework/maths.h"
#include "framework/logger.h"

#define GROWTH_MIN (1 << 3)
#define GROWTH_MUL 3
#define GROWTH_DIV 2

//
#include "internal.h"

uint32_t grow_capacity_value_u32(uint32_t current, uint32_t delta) {
#if defined(GAME_TARGET_DEVELOPMENT) || defined(GAME_TARGET_DEBUG)
	if (current > UINT32_MAX - delta) {
		logger_to_console("growth `%u + %u` will overflow, default to %u\n", current, delta, UINT32_MAX); DEBUG_BREAK();
		return UINT32_MAX;
	}
#endif

	uint32_t const target = current + delta;
	if (target < GROWTH_MIN) { return GROWTH_MIN; }

#if defined(GAME_TARGET_DEVELOPMENT) || defined(GAME_TARGET_DEBUG)
	if (target > mul_div_u32(UINT32_MAX, GROWTH_DIV, GROWTH_MUL)) {
		logger_to_console("growth `%u + %u` will overflow, default to %u\n", current, delta, target); DEBUG_BREAK();
		return target;
	}
#endif

	if (current < GROWTH_MIN) { current = GROWTH_MIN; }
	while (current < target) {
		current = mul_div_u32(current, GROWTH_MUL, GROWTH_DIV);
	}

	return current;
}

uint64_t grow_capacity_value_u64(uint64_t current, uint64_t delta) {
#if defined(GAME_TARGET_DEVELOPMENT) || defined(GAME_TARGET_DEBUG)
	if (current > UINT64_MAX - delta) {
		logger_to_console("growth `%llu + %llu` will overflow, default to %llu\n", current, delta, UINT64_MAX); DEBUG_BREAK();
		return UINT64_MAX;
	}
#endif

	uint64_t const target = current + delta;
	if (target < GROWTH_MIN) { return GROWTH_MIN; }

#if defined(GAME_TARGET_DEVELOPMENT) || defined(GAME_TARGET_DEBUG)
	if (target > mul_div_u64(UINT64_MAX, GROWTH_DIV, GROWTH_MUL)) {
		logger_to_console("growth `%llu + %llu` will overflow, default to %llu\n", current, delta, target); DEBUG_BREAK();
		return target;
	}
#endif

	if (current < GROWTH_MIN) { current = GROWTH_MIN; }
	while (current < target) {
		current = mul_div_u64(current, GROWTH_MUL, GROWTH_DIV);
	}

	return current;
}

bool should_hash_table_grow(uint32_t capacity, uint32_t count) {
	return count >= mul_div_u64(capacity, GROWTH_DIV, GROWTH_MUL);
}

uint32_t adjust_hash_table_capacity_value(uint32_t value) {
#if defined(HASH_TABLE_PO2)
	return round_up_to_PO2_u32(value);
#else
	return value;
#endif
}

#undef GROWTH_MIN
#undef GROWTH_MUL
#undef GROWTH_DIV
