#if !defined(FRAMEWORK_SYSTEMS_DEFER_SYSTEM)
#define FRAMEWORK_SYSTEMS_DEFER_SYSTEM

#include "framework/common.h"

struct Action {
	uint32_t frames;
	struct Handle handle;
	void (* invoke)(struct Handle handle);
};

void defer_system_clear(bool deallocate);

void defer_system_push(struct Action action);
void defer_system_invoke(void);

#endif
