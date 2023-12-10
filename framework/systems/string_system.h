#if !defined(FRAMEWORK_SYSTEMS_STRING_SYSTEM)
#define FRAMEWORK_SYSTEMS_STRING_SYSTEM

#include "framework/common.h"

void string_system_clear(bool deallocate);

uint32_t string_system_add(struct CString value);
uint32_t string_system_find(struct CString value);
struct CString string_system_get(uint32_t id);

#endif
