#if !defined(FRAMEWORK_SYSTEMS_STRINGS)
#define FRAMEWORK_SYSTEMS_STRINGS

#include "framework/common.h"

void system_strings_clear(bool deallocate);

struct Handle system_strings_add(struct CString value);
struct Handle system_strings_find(struct CString value);
struct CString system_strings_get(struct Handle handle);

#endif
