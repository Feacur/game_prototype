#if !defined(GAME_PLATFORM_DEBUG)
#define GAME_PLATFORM_DEBUG

#include "common.h"

struct Callstack {
	uint32_t count;
	uint64_t data[32];
};

struct Callstack platform_debug_get_callstack(void);
struct CString platform_debug_get_stacktrace(struct Callstack callstack, uint32_t offset);

#endif
