#if !defined(FRAMEWORK_CONTAINERS_INTERNAL_HELPERS)
#define FRAMEWORK_CONTAINERS_INTERNAL_HELPERS

#include "framework/common.h"

// @purpose: provide common containers operations

#define HASH_PO2

//

enum Hash_Mark {
	HASH_MARK_NONE,
	HASH_MARK_SKIP,
	HASH_MARK_FULL,
};

//

uint32_t growth_adjust_array(uint32_t current, uint32_t target);
size_t growth_adjust_buffer(size_t current, size_t target);

bool growth_hash_check(uint32_t current, uint32_t target);
uint32_t growth_hash_adjust(uint32_t current, uint32_t target);

//

#if defined(HASH_PO2)
	#define HASH_TABLE_WRAP(value, range) ((value) & ((range) - 1))
#else
	#define HASH_TABLE_WRAP(value, range) ((value) % (range))
#endif

#endif
