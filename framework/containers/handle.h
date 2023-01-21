#if !defined(FRAMEWORK_CONTAINERS_HANDLE)
#define FRAMEWORK_CONTAINERS_HANDLE

#include "framework/common.h"

// @purpose: decouple `handle_table.h` with any other one

struct Handle {
	uint32_t id, gen;
};

inline static bool handle_is_null(struct Handle v) { return v.id == 0; }
inline static bool handle_equals(struct Handle v1, struct Handle v2) {
	return v1.gen == v2.gen && v1.id == v2.id;
}

#endif
