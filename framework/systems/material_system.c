#include "framework/containers/handle_table.h"
#include "framework/graphics/material.h"

#include "material_system.h"

static struct Material_System {
	struct Handle_Table instances;
} gs_material_system;

void material_system_init(void) {
	gs_material_system = (struct Material_System) {
		.instances = handle_table_init(sizeof(struct Gfx_Material)),
	};
}

void material_system_free(void) {
	handle_table_free(&gs_material_system.instances);
	// common_memset(&gs_material_system, 0, sizeof(gs_material_system));
}

struct Handle material_system_aquire(void) {
	struct Gfx_Material value = gfx_material_init();
	return handle_table_aquire(&gs_material_system.instances, &value);
}

void material_system_discard(struct Handle handle) {
	struct Gfx_Material * entry = handle_table_get(&gs_material_system.instances, handle);
	gfx_material_free(entry);
	handle_table_discard(&gs_material_system.instances, handle);
}

struct Gfx_Material * material_system_take(struct Handle handle) {
	return handle_table_get(&gs_material_system.instances, handle);
}
