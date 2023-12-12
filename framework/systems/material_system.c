#include "framework/containers/sparseset.h"
#include "framework/graphics/gfx_material.h"


//
#include "material_system.h"

static struct Material_System {
	struct Sparseset instances; // `struct Gfx_Material`
} gs_material_system = {
	.instances = {
		.packed = {
			.value_size = sizeof(struct Gfx_Material),
		},
		.sparse = {
			.value_size = sizeof(struct Handle),
		},
		.ids = {
			.value_size = sizeof(uint32_t),
		},
	},
};

void material_system_clear(bool deallocate) {
	// dependecies
	FOR_SPARSESET(&gs_material_system.instances, it) {
		gfx_material_free(it.value);
	}
	// personal
	sparseset_clear(&gs_material_system.instances, deallocate);
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

struct Gfx_Material * material_system_get(struct Handle handle) {
	return sparseset_get(&gs_material_system.instances, handle);
}
