#if !defined(FRAMEWORK_SYSTEMS_STRINGS)
#define FRAMEWORK_SYSTEMS_STRINGS

#include "framework/common.h"

void system_strings_init(void);
void system_strings_free(void);

struct Handle system_strings_add(struct CString value);
struct Handle system_strings_find(struct CString value);
struct CString system_strings_get(struct Handle handle);

#endif
