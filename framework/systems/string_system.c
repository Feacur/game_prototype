#include "framework/containers/strings.h"

//
#include "string_system.h"

static struct String_System {
	struct Strings instances;
} gs_string_system;

void string_system_init(void) {
	gs_string_system.instances = strings_init();
	// common_memset(&gs_string_system, 0, sizeof(gs_string_system));
}

void string_system_free(void) {
	strings_free(&gs_string_system.instances);
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
