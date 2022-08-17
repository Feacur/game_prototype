#if !defined(FRAMEWORK_CONTAINERS_INTERNAL)
#define FRAMEWORK_CONTAINERS_INTERNAL

#include "framework/common.h"

#define HASH_TABLE_PO2

//

enum Hash_Table_Mark {
	HASH_TABLE_MARK_NONE,
	HASH_TABLE_MARK_SKIP,
	HASH_TABLE_MARK_FULL,
};

//

uint32_t grow_capacity_value_u32(uint32_t current, uint32_t delta);
uint64_t grow_capacity_value_u64(uint64_t current, uint64_t delta);

bool should_hash_table_grow(uint32_t capacity, uint32_t count);
uint32_t adjust_hash_table_capacity_value(uint32_t value);

//

#if defined(HASH_TABLE_PO2)
	#define HASH_TABLE_WRAP(value, range) ((value) & ((range) - 1))
#else
	#define HASH_TABLE_WRAP(value, range) ((value) % (range))
#endif

#endif
