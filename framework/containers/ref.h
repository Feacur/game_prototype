#if !defined(GAME_CONTAINERS_REF)
#define GAME_CONTAINERS_REF

#include "framework/common.h"

struct Ref {
	uint32_t id, gen;
};

extern struct Ref const c_ref_empty;

#endif
