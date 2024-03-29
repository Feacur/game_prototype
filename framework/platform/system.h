#if !defined(FRAMEWORK_PLATFORM_SYSTEM)
#define FRAMEWORK_PLATFORM_SYSTEM

#include "framework/common.h"

struct Platform_Callbacks {
	void (* quit)(void);
};

void platform_system_init(struct Platform_Callbacks callbacks);
void platform_system_free(void);

bool platform_system_is_error(void);
void platform_system_log_last_error(void);

void platform_system_update(void);
void platform_system_sleep(uint32_t millis);

void * platform_system_get_module(void);

#endif
