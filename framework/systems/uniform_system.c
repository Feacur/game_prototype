#include "framework/containers/strings.h"

//
#include "uniform_system.h"

static struct Uniform_System {
	struct Strings instances;
} gs_uniform_system;

void uniform_system_init(void) {
	gs_uniform_system.instances = strings_init();
	// common_memset(&gs_uniform_system, 0, sizeof(gs_uniform_system));
}

void uniform_system_free(void) {
	strings_free(&gs_uniform_system.instances);
}

uint32_t uniform_system_add(struct CString name) {
	return strings_add(&gs_uniform_system.instances, name);
}

uint32_t uniform_system_find(struct CString name) {
	return strings_find(&gs_uniform_system.instances, name);
}

struct CString uniform_system_get(uint32_t value) {
	return strings_get(&gs_uniform_system.instances, value);
}
