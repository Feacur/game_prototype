#include "framework/containers/strings.h"

//
#include "uniforms.h"

static struct Strings gs_uniforms;

void uniforms_init(void) {
	gs_uniforms = strings_init();
}

void uniforms_free(void) {
	strings_free(&gs_uniforms);
}

uint32_t uniforms_add(struct CString name) {
	return strings_add(&gs_uniforms, name);
}

uint32_t uniforms_find(struct CString name) {
	return strings_find(&gs_uniforms, name);
}

struct CString uniforms_get(uint32_t value) {
	return strings_get(&gs_uniforms, value);
}
