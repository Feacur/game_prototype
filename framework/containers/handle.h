#if !defined(FRAMEWORK_CONTAINERS_HANDLE)
#define FRAMEWORK_CONTAINERS_HANDLE

#include "framework/common.h"

// @purpose: decouple `sparseset.h` and `asset_system.h` with any other one

struct Handle {
	uint32_t id : 24; // 0x00ffffff
	uint32_t gen : 8; // 0x000000ff
};
STATIC_ASSERT(sizeof(struct Handle) == sizeof(uint32_t), handle);

inline static bool handle_is_null(struct Handle v) { return v.id == 0; }
inline static bool handle_equals(struct Handle v1, struct Handle v2) {
	return v1.gen == v2.gen && v1.id == v2.id;
	// return *((uint32_t *)&v1) == *((uint32_t *)&v2);
}

#endif
