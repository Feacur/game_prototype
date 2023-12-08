#if !defined(FRAMEWORK_SYSTEMS_ACTION_SYSTEM)
#define FRAMEWORK_SYSTEMS_ACTION_SYSTEM

#include "framework/common.h"

struct Action {
	struct Handle handle;
	void (* action)(struct Handle handle);
};

void action_system_init(void);
void action_system_free(void);

void action_system_push(struct Action action);
void action_system_update(void);

#endif
