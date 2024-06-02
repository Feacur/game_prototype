#include "framework/containers/sparseset.h"
#include "framework/graphics/gfx_material.h"


//
#include "materials.h"

static struct Materials {
	struct Sparseset instances; // `struct Gfx_Material`
} gs_materials;

void system_materials_init(void) {
	gs_materials = (struct Materials){
		.instances = {
			.payload = {
				.value_size = sizeof(struct Gfx_Material),
			},
			.sparse = {
				.value_size = sizeof(struct Handle),
			},
			.packed = {
				.value_size = sizeof(uint32_t),
			},
		},
	};
}

void system_materials_free(void) {
	FOR_SPARSESET(&gs_materials.instances, it) {
		gfx_material_free(it.value);
	}
	sparseset_free(&gs_materials.instances);
}

struct Handle system_materials_aquire(void) {
	struct Gfx_Material value = gfx_material_init();
	return sparseset_aquire(&gs_materials.instances, &value);
}

HANDLE_ACTION(system_materials_discard) {
	struct Gfx_Material * entry = sparseset_get(&gs_materials.instances, handle);
	if (entry != NULL) {
		gfx_material_free(entry);
		sparseset_discard(&gs_materials.instances, handle);
	}
}

struct Gfx_Material * system_materials_get(struct Handle handle) {
	return sparseset_get(&gs_materials.instances, handle);
}
