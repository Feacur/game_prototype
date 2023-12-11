#include "framework/containers/strings.h"


//
#include "string_system.h"

static struct String_System {
	struct Strings instances;
} gs_string_system = {
	.instances = {
		.offsets = {
			.value_size = sizeof(uint32_t),
		},
		.lengths = {
			.value_size = sizeof(uint32_t),
		},
	},
};

void string_system_clear(bool deallocate) {
	strings_clear(&gs_string_system.instances, deallocate);
}

uint32_t string_system_add(struct CString value) {
	return strings_add(&gs_string_system.instances, value);
}

uint32_t string_system_find(struct CString value) {
	return strings_find(&gs_string_system.instances, value);
}

struct CString string_system_get(uint32_t id) {
	return strings_get(&gs_string_system.instances, id);
}
