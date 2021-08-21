#include "framework/logger.h"
#include "framework/maths.h"

#include "framework/containers/array_any.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"

#include "game_state.h"

#include <math.h>

//
#include "object_entity.h"

void entity_get_rect(
	struct Transform_3D const * transform, struct Transform_Rect const * rect,
	uint32_t camera_size_x, uint32_t camera_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
) {
	*min = (struct vec2){
		rect->min_relative.x * (float)camera_size_x + rect->min_absolute.x,
		rect->min_relative.y * (float)camera_size_y + rect->min_absolute.y,
	};
	*max = (struct vec2){
		rect->max_relative.x * (float)camera_size_x + rect->max_absolute.x,
		rect->max_relative.y * (float)camera_size_y + rect->max_absolute.y,
	};
	*pivot = (struct vec2){
		.x = lerp(min->x, max->x, rect->pivot.x) + transform->position.x,
		.y = lerp(min->y, max->y, rect->pivot.y) + transform->position.y,
	};
}

struct uvec2 entity_get_content_size(
	struct Entity const * entity,
	uint32_t camera_size_x, uint32_t camera_size_y
) {
	struct Gfx_Material * material = array_any_at(&state.materials, entity->material);

	switch (entity->type) {
		case ENTITY_TYPE_MESH: return (struct uvec2){
			camera_size_x,
			camera_size_y
		};

		case ENTITY_TYPE_QUAD_2D: {
			struct Entity_Quad const * quad = &entity->as.quad;

			struct Ref const * gpu_texture_ref = gfx_material_get_texture(material, quad->texture_uniform);

			uint32_t texture_size_x, texture_size_y;
			gpu_texture_get_size(*gpu_texture_ref, &texture_size_x, &texture_size_y);

			return (struct uvec2){
				texture_size_x,
				texture_size_y
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			int32_t const rect[] = {
				(int32_t)floorf(entity->rect.min_relative.x * (float)camera_size_x + entity->rect.min_absolute.x),
				(int32_t)floorf(entity->rect.min_relative.y * (float)camera_size_y + entity->rect.min_absolute.y),
				(int32_t)ceilf(entity->rect.max_relative.x * (float)camera_size_x + entity->rect.max_absolute.x),
				(int32_t)ceilf(entity->rect.max_relative.y * (float)camera_size_y + entity->rect.max_absolute.y),
			};
			return (struct uvec2){ // @idea: invert negatives?
				(uint32_t)max_s32(rect[2] - rect[0], 0),
				(uint32_t)max_s32(rect[3] - rect[1], 0),
			};
		} // break;
	}

	logger_to_console("unknown entity type"); DEBUG_BREAK();
	return (struct uvec2){0};
}
