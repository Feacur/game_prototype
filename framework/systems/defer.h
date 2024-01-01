#if !defined(FRAMEWORK_SYSTEMS_DEFERS)
#define FRAMEWORK_SYSTEMS_DEFERS

#include "framework/common.h"

struct Action {
	uint32_t frames;
	struct Handle handle;
	Handle_Action * invoke;
};

void system_defer_clear(bool deallocate);

void system_defer_push(struct Action action);
void system_defer_invoke(void);

#endif
