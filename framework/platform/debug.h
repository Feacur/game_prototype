#if !defined(FRAMEWORK_PLATFORM_DEBUG)
#define FRAMEWORK_PLATFORM_DEBUG

#include "framework/common.h"

struct Callstack platform_debug_get_callstack(uint32_t skip);
struct CString platform_debug_get_stacktrace(struct Callstack callstack);

#endif
