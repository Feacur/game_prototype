#if !defined(FRAMEWORK_SYSTEMS_BUFFER_SYSTEM)
#define FRAMEWORK_SYSTEMS_BUFFER_SYSTEM

#include "framework/common.h"

void buffer_system_init(void);
void buffer_system_free(void);

void * buffer_system_push(size_t size);
void buffer_system_pop(void * pointer);
void buffer_system_reset(void);

#endif
