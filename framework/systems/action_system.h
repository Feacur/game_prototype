#if !defined(FRAMEWORK_SYSTEMS_ACTION_SYSTEM)
#define FRAMEWORK_SYSTEMS_ACTION_SYSTEM

#include "framework/common.h"

struct Action {
	uint32_t frames;
	struct Handle handle;
	void (* invoke)(struct Handle handle);
};

void action_system_clear(bool deallocate);

void action_system_push(struct Action action);
void action_system_invoke(void);

#endif
