#if !defined(FRAMEWORK_CONTAINERS_REF)
#define FRAMEWORK_CONTAINERS_REF

#include "framework/common.h"

// @purpose: decouple `ref_table.h` with any other one

struct Ref {
	uint32_t id, gen;
};

inline static bool ref_equals(struct Ref v1, struct Ref v2) {
	return v1.gen == v2.gen && v1.id == v2.id;
}

extern struct Ref const c_ref_empty;

#endif
