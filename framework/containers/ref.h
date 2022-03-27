#if !defined(GAME_CONTAINERS_REF)
#define GAME_CONTAINERS_REF

#include "framework/common.h"

struct Ref {
	uint32_t id, gen;
};

inline static bool ref_equals(struct Ref v1, struct Ref v2) {
	return v1.gen == v2.gen && v1.id == v2.id;
}

extern struct Ref const c_ref_empty;

#endif
