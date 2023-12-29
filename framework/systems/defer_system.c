#include "framework/containers/array.h"


//
#include "defer_system.h"

static struct Defer_System {
	struct Array actions; // `struct Action`
} gs_defer_system = {
	.actions = {
		.value_size = sizeof(struct Action),
	},
};

static PREDICATE(action_is_empty) {
	struct Action const * action = value;
	return handle_is_null(action->handle)
	    || action->invoke == NULL;
}

void defer_system_clear(bool deallocate) {
	// dependencies
	FOR_ARRAY(&gs_defer_system.actions, it) {
		if (action_is_empty(it.value)) { continue; }
		struct Action * action = it.value;
		action->invoke(action->handle);
	}
	// personal
	array_clear(&gs_defer_system.actions, deallocate);
}

void defer_system_push(struct Action action) {
	array_push_many(&gs_defer_system.actions, 1, &action);
}

void defer_system_invoke(void) {
	array_remove_if(&gs_defer_system.actions, action_is_empty);
	FOR_ARRAY(&gs_defer_system.actions, it) {
		struct Action * action = it.value;
		if (action->frames > 0) { action->frames--; continue; }
		action->invoke(action->handle);
		action->handle = (struct Handle){0};
	}
}
