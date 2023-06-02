#include "framework/logger.h"
#include "framework/maths.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"

#include "framework/systems/material_system.h"

//
#include "object_entity.h"

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Handle material_handle,
	uint32_t parent_size_x, uint32_t parent_size_y
) {
	switch (entity->type) {
		case ENTITY_TYPE_NONE: return (struct uvec2){0, 0};

		case ENTITY_TYPE_MESH: return (struct uvec2){
			parent_size_x,
			parent_size_y
		};

		case ENTITY_TYPE_QUAD_2D: {
			struct Entity_Quad const * quad = &entity->as.quad;

			struct Gfx_Material const * material = material_system_take(material_handle);
			struct CArray_Mut const field = gfx_uniforms_id_get(&material->uniforms, quad->uniform_id, 0);

			if (field.data == NULL) { return (struct uvec2){0, 0}; }
			struct Handle const gpu_texture_handle = *(struct Handle *)field.data;

			uint32_t texture_size_x, texture_size_y;
			gpu_texture_get_size(gpu_texture_handle, &texture_size_x, &texture_size_y);

			return (struct uvec2){
				(uint32_t)r32_floor((float)texture_size_x * clamp_r32(quad->view.max.x - quad->view.min.x, 0, 1)),
				(uint32_t)r32_floor((float)texture_size_y * clamp_r32(quad->view.max.y - quad->view.min.y, 0, 1)),
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			struct srect const rect = {
				.min = {
					(int32_t)r32_floor((float)parent_size_x * entity->rect.anchor_min.x + entity->rect.extents.x),
					(int32_t)r32_floor((float)parent_size_y * entity->rect.anchor_min.y + entity->rect.extents.y),
				},
				.max = {
					(int32_t)r32_ceil ((float)parent_size_x * entity->rect.anchor_max.x + entity->rect.extents.x),
					(int32_t)r32_ceil ((float)parent_size_y * entity->rect.anchor_max.y + entity->rect.extents.y),
				},
			};
			return (struct uvec2){
				(uint32_t)max_s32(rect.max.x - rect.min.x, rect.min.x - rect.max.x),
				(uint32_t)max_s32(rect.max.y - rect.min.y, rect.min.y - rect.max.y),
			};
		} // break;
	}

	logger_to_console("unknown entity type\n"); DEBUG_BREAK();
	return (struct uvec2){0};
}
