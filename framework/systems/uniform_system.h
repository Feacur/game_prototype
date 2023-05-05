#if !defined(FRAMEWORK_SYSTEMS_UNIFORM_SYSTEM)
#define FRAMEWORK_SYSTEMS_UNIFORM_SYSTEM

#include "framework/common.h"

void uniform_system_init(void);
void uniform_system_free(void);

uint32_t uniform_system_add(struct CString name);
uint32_t uniform_system_find(struct CString name);
struct CString uniform_system_get(uint32_t value);

#endif
