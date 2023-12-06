#include "framework/containers/sparseset.h"
#include "framework/graphics/gfx_material.h"

#include "material_system.h"

static struct Material_System {
	struct Sparseset instances;
} gs_material_system;

void material_system_init(void) {
	gs_material_system = (struct Material_System) {
		.instances = sparseset_init(sizeof(struct Gfx_Material)),
	};
}

void material_system_free(void) {
	sparseset_free(&gs_material_system.instances);
	// common_memset(&gs_material_system, 0, sizeof(gs_material_system));
}

struct Handle material_system_aquire(void) {
	struct Gfx_Material value = gfx_material_init();
	return sparseset_aquire(&gs_material_system.instances, &value);
}

void material_system_discard(struct Handle handle) {
	struct Gfx_Material * entry = sparseset_get(&gs_material_system.instances, handle);
	if (entry != NULL) {
		gfx_material_free(entry);
		sparseset_discard(&gs_material_system.instances, handle);
	}
}

struct Gfx_Material * material_system_take(struct Handle handle) {
	return sparseset_get(&gs_material_system.instances, handle);
}
