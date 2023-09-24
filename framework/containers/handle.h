#if !defined(FRAMEWORK_CONTAINERS_HANDLE)
#define FRAMEWORK_CONTAINERS_HANDLE

#include "framework/common.h"

// @purpose: decouple `sparseset.h` and `asset_system.h` with any other one

struct Handle {
	uint32_t id : 24; // 0x00ffffff
	uint32_t gen : 8; // 0x000000ff
};
STATIC_ASSERT(sizeof(struct Handle) == sizeof(uint32_t), handle);

inline static bool handle_is_null(struct Handle h) { return h.id == 0; }
inline static bool handle_equals(struct Handle h1, struct Handle h2) {
	return h1.gen == h2.gen && h1.id == h2.id;
}

#endif
