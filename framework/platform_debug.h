#if !defined(FRAMEWORK_PLATFORM_DEBUG)
#define FRAMEWORK_PLATFORM_DEBUG

#include "common.h"

struct Callstack {
	uint32_t count;
	uint64_t data[64];
};

struct Callstack platform_debug_get_callstack(uint32_t skip);
struct CString platform_debug_get_stacktrace(struct Callstack callstack);

#endif
