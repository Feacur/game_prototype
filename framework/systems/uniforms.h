#if !defined(FRAMEWORK_SYSTEMS_UNIFORMS)
#define FRAMEWORK_SYSTEMS_UNIFORMS

#include "framework/common.h"

void uniforms_init(void);
void uniforms_free(void);

uint32_t uniforms_add(struct CString name);
uint32_t uniforms_find(struct CString name);
struct CString uniforms_get(uint32_t value);

#endif
