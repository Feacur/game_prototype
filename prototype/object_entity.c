#include "framework/formatter.h"
#include "framework/maths.h"

#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"

#include "framework/graphics/gfx_material.h"
#include "framework/graphics/gfx_objects.h"


//
#include "object_entity.h"

struct Entity entity_init(void) {
	return (struct Entity){0};
}

void entity_free(struct Entity * entity) {
	switch (entity->type) {
		default: break;

		case ENTITY_TYPE_MESH:
			asset_system_drop(entity->as.mesh.ah_mesh);
			break;

		case ENTITY_TYPE_TEXT_2D:
			asset_system_drop(entity->as.text.ah_font);
			asset_system_drop(entity->as.text.ah_text);
			break;
	}
	asset_system_drop(entity->ah_material);
}

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Handle material_handle,
	struct uvec2 parent_size
) {
	switch (entity->type) {
		case ENTITY_TYPE_NONE: return (struct uvec2){0, 0};

		case ENTITY_TYPE_MESH: return parent_size;

		case ENTITY_TYPE_QUAD_2D: {
			struct Entity_Quad const * e_quad = &entity->as.quad;

			struct Gfx_Material const * material = material_system_get(material_handle);
			struct CArray_Mut const field = gfx_uniforms_id_get(&material->uniforms, e_quad->sh_uniform, 0);

			if (field.data == NULL) { return (struct uvec2){0, 0}; }
			struct Handle const gh_texture = *(struct Handle *)field.data;

			struct GPU_Texture const * texture = gpu_texture_get(gh_texture);
			struct uvec2 const texture_size = (texture != NULL) ? texture->size : (struct uvec2){0};

			return (struct uvec2){
				(uint32_t)r32_floor((float)texture_size.x * clamp_r32(e_quad->view.max.x - e_quad->view.min.x, 0, 1)),
				(uint32_t)r32_floor((float)texture_size.y * clamp_r32(e_quad->view.max.y - e_quad->view.min.y, 0, 1)),
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			struct srect const rect = {
				.min = {
					(int32_t)r32_floor((float)parent_size.x * entity->rect.anchor_min.x + entity->rect.extents.x),
					(int32_t)r32_floor((float)parent_size.y * entity->rect.anchor_min.y + entity->rect.extents.y),
				},
				.max = {
					(int32_t)r32_ceil ((float)parent_size.x * entity->rect.anchor_max.x + entity->rect.extents.x),
					(int32_t)r32_ceil ((float)parent_size.y * entity->rect.anchor_max.y + entity->rect.extents.y),
				},
			};
			return (struct uvec2){
				(uint32_t)max_s32(rect.max.x - rect.min.x, rect.min.x - rect.max.x),
				(uint32_t)max_s32(rect.max.y - rect.min.y, rect.min.y - rect.max.y),
			};
		} // break;
	}

	ERR("unknown entity type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct uvec2){0};
}
