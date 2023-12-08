#include "framework/containers/array.h"

#include "action_system.h"

static struct Action_System {
	struct Array actions; // `struct Action`
} gs_action_system;

void action_system_init(void) {
	gs_action_system = (struct Action_System) {
		.actions = array_init(sizeof(struct Action)),
	};
}

void action_system_free(void) {
	action_system_update();
	array_free(&gs_action_system.actions);
	// common_memset(&gs_action_system, 0, sizeof(gs_action_system));
}

void action_system_push(struct Action action) {
	array_push_many(&gs_action_system.actions, 1, &action);
}

void action_system_update(void) {
	FOR_ARRAY(&gs_action_system.actions, it) {
		struct Action const * entry = it.value;
		entry->action(entry->handle);
	}
	array_clear(&gs_action_system.actions);
}
