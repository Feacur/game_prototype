#include "framework/containers/array.h"


//
#include "defer.h"

static struct Defers {
	struct Array actions; // `struct Action`
} gs_defer;

static PREDICATE(action_is_empty) {
	struct Action const * action = value;
	return handle_is_null(action->handle)
	    || action->invoke == NULL;
}

void system_defer_init(void) {
	gs_defer = (struct Defers){
		.actions = {
			.value_size = sizeof(struct Action),
		},
	};
}

void system_defer_free(void) {
	FOR_ARRAY(&gs_defer.actions, it) {
		if (action_is_empty(it.value)) { continue; }
		struct Action * action = it.value;
		action->invoke(action->handle);
	}
	array_free(&gs_defer.actions);
}

void system_defer_push(struct Action action) {
	array_push_many(&gs_defer.actions, 1, &action);
}

void system_defer_invoke(void) {
	array_remove_if(&gs_defer.actions, action_is_empty);
	FOR_ARRAY(&gs_defer.actions, it) {
		struct Action * action = it.value;
		if (action->frames > 0) { action->frames--; continue; }
		action->invoke(action->handle);
		action->handle = (struct Handle){0};
	}
}
